// Android: the game's network side, the same protocol as the Cardputer's
// diffusion.cpp so that mission control (a web browser) works unchanged.
//
// Android itself is connected (Wi-Fi settings, or the OUYA's Ethernet); the
// game never scans, joins or creates a network (no access point: user's
// choice). An app without root cannot open ports 80 and 81, so:
//   http://<address>:8080/   mission control's page (the Cardputer's page,
//                            copied into page.inc by tools/build.sh)
//   ws://<address>:8081/     the stream: changed screen rows, sounds, pages
// One network thread serves both, like the Cardputer's network task.
#include <diffusion.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

#include "display.h"
#include "network.h"
#include "sha1.h"

// The Cardputer's page: const char PAGE[] (made by tools/build.sh)
#include "page.inc"

// ---------------------------------------------------------------- network

namespace network {
namespace {
std::mutex lock;
std::string currentName;
std::string currentIp;
}  // namespace

void set(const std::string &n, const std::string &i) {
    std::lock_guard<std::mutex> g(lock);
    currentName = n;
    currentIp = i;
}

std::string name() {
    std::lock_guard<std::mutex> g(lock);
    return currentName;
}

std::string ip() {
    std::lock_guard<std::mutex> g(lock);
    return currentIp;
}
}  // namespace network

namespace mirror {
namespace {

constexpr int HTTP_PORT = 8080;
constexpr int WS_PORT = 8081;
constexpr int MAX_CLIENTS = 5;            // as on the Cardputer
constexpr uint32_t FRAME_MS = 40;         // 25 pictures/s at most
constexpr uint32_t CLOCK_MS = 250;        // game clock for the sound
constexpr uint32_t PING_MS = 5000;        // heartbeat, as on the Cardputer
constexpr int MISSED_PONGS = 2;
constexpr size_t FRAME_BUF = 12 * 1024;   // a picture may go in several messages
constexpr int MAX_H = 135;

struct Client {
    int fd = -1;
    bool open = false;  // handshake done
    std::string in;     // bytes received, not yet used
    int missed = 0;     // pings without a pong
};

std::mutex screenMutex;   // the game's canvas, drawn by the game thread
std::mutex stateLock;     // everything below that the game thread sets
const uint16_t *screenBuf = nullptr;
int screenW = 0;
int screenH = 0;
std::deque<std::string> soundMsgs;  // "T,...", "S,...", "R,..." without the current time
std::string panelText = "0";
bool panelDirty = false;
std::string bannerText;
int bannerY = 0;
int bannerH = 0;
bool bannerDirty = false;
uint8_t pageLang = 0;
bool langDirty = false;
std::string symbolsJs = "const SYM=[];";
std::atomic<int> clients(0);
bool started = false;

// Network thread only
Client conns[MAX_CLIENTS];
uint32_t rowHash[MAX_H];  // 0 = row to send again
int nextRow = 0;
int bandY = 0;
int bandH = 0;
uint8_t frameBuf[FRAME_BUF];

int listenOn(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (sockaddr *)&a, sizeof a) < 0 || listen(fd, 8) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool sendAll(int fd, const void *data, size_t len) {
    const char *p = static_cast<const char *>(data);
    while (len > 0) {
        ssize_t n = send(fd, p, len, MSG_NOSIGNAL);
        if (n <= 0) {
            return false;
        }
        p += n;
        len -= n;
    }
    return true;
}

// ---- HTTP: the page and /sym.js, one request per connection

void serveHttp(int fd) {
    timeval tv = {2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    std::string req;
    char buf[1024];
    while (req.find("\r\n\r\n") == std::string::npos && req.size() < 8192) {
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n <= 0) {
            break;
        }
        req.append(buf, n);
    }
    std::string path;
    size_t sp = req.find(' ');
    if (sp != std::string::npos) {
        size_t sp2 = req.find(' ', sp + 1);
        path = req.substr(sp + 1, sp2 == std::string::npos ? std::string::npos : sp2 - sp - 1);
    }
    size_t q = path.find('?');
    if (q != std::string::npos) {
        path.erase(q);
    }
    std::string type = "text/plain";
    std::string body = "404";
    const char *status = "404 Not Found";
    if (path == "/") {
        type = "text/html; charset=utf-8";
        body = PAGE;
        status = "200 OK";
    } else if (path == "/sym.js") {
        type = "application/javascript";
        std::lock_guard<std::mutex> g(stateLock);
        body = symbolsJs;
        status = "200 OK";
    }
    std::string head = std::string("HTTP/1.1 ") + status + "\r\nContent-Type: " + type +
                       "\r\nContent-Length: " + std::to_string(body.size()) +
                       "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    sendAll(fd, head.data(), head.size());
    sendAll(fd, body.data(), body.size());
    close(fd);
}

// ---- WebSocket

bool sendFrame(Client &c, int opcode, const void *data, size_t len) {
    uint8_t head[4];
    size_t h = 2;
    head[0] = 0x80 | opcode;
    if (len < 126) {
        head[1] = (uint8_t)len;
    } else {
        head[1] = 126;  // payloads stay below 64 KB
        head[2] = (uint8_t)(len >> 8);
        head[3] = (uint8_t)len;
        h = 4;
    }
    return sendAll(c.fd, head, h) && sendAll(c.fd, data, len);
}

void dropClient(Client &c) {
    if (c.fd >= 0) {
        close(c.fd);
    }
    c = Client();
    int n = 0;
    for (const Client &k : conns) {
        n += k.open ? 1 : 0;
    }
    clients = n;
}

void broadcast(int opcode, const void *data, size_t len) {
    for (Client &c : conns) {
        if (c.open && !sendFrame(c, opcode, data, len)) {
            dropClient(c);
        }
    }
}

void broadcastText(const std::string &s) {
    broadcast(1, s.data(), s.size());
}

std::string header(const std::string &req, const char *name) {
    std::string lower = req;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string key = std::string("\r\n") + name + ":";
    size_t p = lower.find(key);
    if (p == std::string::npos) {
        return "";
    }
    p += key.size();
    size_t e = req.find("\r\n", p);
    std::string v = req.substr(p, e - p);
    v.erase(0, v.find_first_not_of(' '));
    v.erase(v.find_last_not_of(' ') + 1);
    return v;
}

// A new browser: the whole screen, its page, its language and the banner
void welcome() {
    memset(rowHash, 0, sizeof rowHash);
    std::lock_guard<std::mutex> g(stateLock);
    panelDirty = langDirty = bannerDirty = true;
}

bool handshake(Client &c) {
    size_t end = c.in.find("\r\n\r\n");
    if (end == std::string::npos) {
        return c.in.size() < 8192;  // wait for the rest
    }
    std::string req = c.in.substr(0, end + 2);
    c.in.erase(0, end + 4);
    std::string key = header(req, "sec-websocket-key");
    if (key.empty()) {
        return false;
    }
    std::string accept = sha1::base64(sha1::digest(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
    std::string resp = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                       "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    if (!sendAll(c.fd, resp.data(), resp.size())) {
        return false;
    }
    c.open = true;
    int n = 0;
    for (const Client &k : conns) {
        n += k.open ? 1 : 0;
    }
    clients = n;
    welcome();
    return true;
}

// Frames from the browser: answers pings, notes pongs, closes on request
bool readFrames(Client &c) {
    for (;;) {
        if (c.in.size() < 2) {
            return true;
        }
        const uint8_t *b = reinterpret_cast<const uint8_t *>(c.in.data());
        int opcode = b[0] & 0x0F;
        bool masked = b[1] & 0x80;
        uint64_t len = b[1] & 0x7F;
        size_t pos = 2;
        if (len == 126) {
            if (c.in.size() < 4) return true;
            len = (b[2] << 8) | b[3];
            pos = 4;
        } else if (len == 127) {
            return false;  // never sent by mission control
        }
        size_t need = pos + (masked ? 4 : 0) + len;
        if (c.in.size() < need) {
            return need < 65536;
        }
        std::string payload = c.in.substr(pos + (masked ? 4 : 0), len);
        if (masked) {
            for (size_t i = 0; i < payload.size(); i++) {
                payload[i] ^= b[pos + (i & 3)];
            }
        }
        c.in.erase(0, need);
        if (opcode == 8) {
            return false;
        } else if (opcode == 9) {
            if (!sendFrame(c, 10, payload.data(), payload.size())) return false;
        } else if (opcode == 10) {
            c.missed = 0;
        }
    }
}

// ---- Screen rows, as the Cardputer sends them

uint32_t hashRow(const uint16_t *p) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < screenW; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h | 1;
}

// Row y raw or run-length encoded, whichever is shorter; 0 if it does not fit
size_t encodeRow(int y, uint8_t *out, size_t room) {
    const uint16_t *p = screenBuf + y * screenW;
    size_t rle = 2;
    for (int x = 0; x < screenW;) {
        int n = 1;
        while (x + n < screenW && n < 255 && p[x + n] == p[x]) n++;
        rle += 3;
        x += n;
    }
    size_t raw = 2 + screenW * 2;
    size_t need = std::min(rle, raw);
    if (need > room) {
        return 0;
    }
    out[0] = y;
    if (rle < raw) {
        out[1] = 1;
        uint8_t *o = out + 2;
        for (int x = 0; x < screenW;) {
            int n = 1;
            while (x + n < screenW && n < 255 && p[x + n] == p[x]) n++;
            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(p + x);
            *o++ = n;
            *o++ = bytes[0];  // the sprite's byte order: high byte first
            *o++ = bytes[1];
            x += n;
        }
    } else {
        out[1] = 0;
        memcpy(out + 2, p, screenW * 2);
    }
    return need;
}

void sendScreen() {
    size_t len = 0;
    int stopAt = -1;
    {
        std::lock_guard<std::mutex> g(screenMutex);
        if (!screenBuf) {
            return;
        }
        for (int i = 0; i < screenH; i++) {
            int y = (nextRow + i) % screenH;
            if (y >= bandY && y < bandY + bandH) {
                continue;  // banner drawn by the page
            }
            uint32_t h = hashRow(screenBuf + y * screenW);
            if (h == rowHash[y]) {
                continue;
            }
            size_t n = encodeRow(y, frameBuf + len, FRAME_BUF - len);
            if (n == 0) {
                stopAt = y;
                break;
            }
            len += n;
            rowHash[y] = h;
        }
    }
    nextRow = stopAt < 0 ? 0 : stopAt;
    if (len > 0) {
        broadcast(2, frameBuf, len);
    }
}

// Pages, language, banner and sounds set by the game
void sendState(uint32_t now) {
    std::deque<std::string> sounds;
    std::string panel, banner;
    bool sendPanel, sendLang, sendBanner;
    int by, bh;
    uint8_t lang;
    {
        std::lock_guard<std::mutex> g(stateLock);
        sounds.swap(soundMsgs);
        sendPanel = panelDirty;
        sendLang = langDirty;
        sendBanner = bannerDirty;
        panelDirty = langDirty = bannerDirty = false;
        panel = panelText;
        banner = bannerText;
        by = bannerY;
        bh = bannerH;
        lang = pageLang;
    }
    std::string t = std::to_string(now);
    for (const std::string &m : sounds) {  // "T,at,..." -> "T,now,at,..."
        broadcastText(m.substr(0, 2) + t + "," + m.substr(2));
    }
    if (sendLang) {
        broadcastText("L," + t + "," + (lang ? "en" : "fr"));
    }
    if (sendPanel) {
        broadcastText("K," + t + "," + panel);
    }
    if (sendBanner) {
        bool on = !banner.empty();
        if (!on) {
            for (int i = bandY; i < bandY + bandH && i < MAX_H; i++) {
                rowHash[i] = 0;  // the banner's rows are sent again
            }
        }
        bandY = on ? by : 0;
        bandH = on ? bh : 0;
        broadcastText("B," + t + "," + std::to_string(by) + "," + std::to_string(bh) + "," + banner);
    }
}

void *netThread(void *) {
    int httpFd = listenOn(HTTP_PORT);
    int wsFd = listenOn(WS_PORT);
    uint32_t nextFrame = 0, nextClock = 0, nextPing = 0;
    for (;;) {
        fd_set rd;
        FD_ZERO(&rd);
        int maxFd = -1;
        if (httpFd >= 0) { FD_SET(httpFd, &rd); maxFd = std::max(maxFd, httpFd); }
        if (wsFd >= 0) { FD_SET(wsFd, &rd); maxFd = std::max(maxFd, wsFd); }
        for (const Client &c : conns) {
            if (c.fd >= 0) { FD_SET(c.fd, &rd); maxFd = std::max(maxFd, c.fd); }
        }
        timeval tv = {0, 5000};
        int r = maxFd >= 0 ? select(maxFd + 1, &rd, nullptr, nullptr, &tv) : 0;
        if (maxFd < 0) {
            usleep(500000);  // ports refused: try again later
            httpFd = httpFd < 0 ? listenOn(HTTP_PORT) : httpFd;
            wsFd = wsFd < 0 ? listenOn(WS_PORT) : wsFd;
            continue;
        }
        if (r > 0) {
            if (httpFd >= 0 && FD_ISSET(httpFd, &rd)) {
                int fd = accept(httpFd, nullptr, nullptr);
                if (fd >= 0) {
                    serveHttp(fd);
                }
            }
            if (wsFd >= 0 && FD_ISSET(wsFd, &rd)) {
                int fd = accept(wsFd, nullptr, nullptr);
                if (fd >= 0) {
                    Client *slot = nullptr;
                    for (Client &c : conns) {
                        if (c.fd < 0) { slot = &c; break; }
                    }
                    if (!slot) {
                        close(fd);  // 5 browsers at most
                    } else {
                        int one = 1;
                        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
                        timeval st = {3, 0};
                        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &st, sizeof st);
                        slot->fd = fd;
                    }
                }
            }
            for (Client &c : conns) {
                if (c.fd < 0 || !FD_ISSET(c.fd, &rd)) {
                    continue;
                }
                char buf[2048];
                ssize_t n = recv(c.fd, buf, sizeof buf, 0);
                if (n <= 0) {
                    dropClient(c);
                    continue;
                }
                c.in.append(buf, n);
                bool ok = c.open ? readFrames(c) : handshake(c) && readFrames(c);
                if (!ok) {
                    dropClient(c);
                }
            }
        }
        uint32_t now = millis();
        if (clients == 0) {
            std::lock_guard<std::mutex> g(stateLock);
            soundMsgs.clear();  // nobody to play them
            continue;
        }
        sendState(now);
        if ((int32_t)(now - nextClock) >= 0) {
            nextClock = now + CLOCK_MS;
            broadcastText("P," + std::to_string(now));
        }
        if ((int32_t)(now - nextPing) >= 0) {
            nextPing = now + PING_MS;
            for (Client &c : conns) {
                if (!c.open) continue;
                if (++c.missed > MISSED_PONGS || !sendFrame(c, 9, "", 0)) {
                    dropClient(c);  // browser gone without closing (Wi-Fi off, console off)
                }
            }
        }
        if ((int32_t)(now - nextFrame) >= 0) {
            nextFrame = now + FRAME_MS;
            sendScreen();
        }
    }
    return nullptr;
}

void queueSound(const std::string &m) {
    std::lock_guard<std::mutex> g(stateLock);
    if (clients > 0 && soundMsgs.size() < 256) {
        soundMsgs.push_back(m);
    }
}

}  // namespace

void startScan() {}

Scan pollScan(std::vector<WifiNet> &out) {
    out.clear();
    return Scan::Done;
}

void connect(const String &, const String &) {}
Link link() { return network::ip().empty() ? Link::Connecting : Link::Connected; }
Link failure() { return Link::NoAnswer; }  // the game then shows its Wi-Fi list, which input.cpp leaves at once
int attempt() { return 1; }

bool startAccessPoint() { return false; }
bool accessPoint() { return false; }
int apClients() { return 0; }
String wifiQrText() { return String(); }

// The address mission control opens (QR code and text of the "connect the screen" screen)
String ipAddress() {
    std::string ip = network::ip();
    return String(ip.empty() ? ip : ip + ":" + std::to_string(HTTP_PORT));
}

bool startServer(const uint16_t *screen, int w, int h) {
    std::lock_guard<std::mutex> g(screenMutex);
    screenBuf = screen;
    screenW = w;
    screenH = std::min(h, MAX_H);
    if (!started) {
        started = true;
        pthread_t t;
        pthread_create(&t, nullptr, netThread, nullptr);
        pthread_detach(t);
    }
    return true;
}

int clientCount() {
    return clients;
}

// The game draws its canvas between these two calls
void lockScreen() {
    screenMutex.lock();
}

void unlockScreen() {
    screenMutex.unlock();
    display::publish();  // right after the frame is drawn
}

void sendTone(uint32_t at, uint16_t freq, uint16_t dur, uint8_t ch) {
    queueSound("T," + std::to_string(at) + "," + std::to_string(freq) + "," + std::to_string(dur) + "," +
               std::to_string(ch));
}

void sendStop(uint32_t at, uint8_t ch) {
    queueSound("S," + std::to_string(at) + "," + std::to_string(ch));
}

void sendRumble(uint32_t at) {
    queueSound("R," + std::to_string(at));
}

void setPanel(const String &text) {
    std::lock_guard<std::mutex> g(stateLock);
    panelText = text.c_str();
    panelDirty = true;
}

void setBanner(const String &text, int y, int h) {
    std::lock_guard<std::mutex> g(stateLock);
    bannerText = text.c_str();
    bannerY = y;
    bannerH = h;
    bannerDirty = true;
}

void setLanguage(uint8_t lang) {
    std::lock_guard<std::mutex> g(stateLock);
    pageLang = lang;
    langDirty = true;
}

void setSymbols(const char *js) {
    std::lock_guard<std::mutex> g(stateLock);
    symbolsJs = js;
}

}  // namespace mirror
