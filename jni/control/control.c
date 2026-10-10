/* Mission control on Android: the app as the crew's screen, in place of a
 * web browser, like GC-Explorer3 on the GameCube. GC-Explorer3's pure code
 * (protocol, pages, banner, sound) is compiled unchanged from its folder;
 * this file is its main loop for Android (see GC-Explorer3/source/gc/main.c).
 *
 *   NO_NETWORK  Android is not connected (its Wi-Fi settings are used)
 *   SEARCH      looking for the crew on the local network (user's choice);
 *               one found: connect; several: LIST; none: NONE
 *   ADDRESS     typed address (fallback), checked, then CONNECTING
 *   CONNECTING  up to 30 s
 *   ERROR       why, and what to do
 *   STREAM      screen copy, pages and sound; lost connection or 30 s of
 *               silence: reconnects by itself, the picture stays
 *
 * Everything runs on the GL thread (Control.frame), except the search
 * (net.c) and the sound (Control.mix, audio thread): the session is shared
 * with the sound under a lock. Java calls come from the UI thread. */
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "draw.h"
#include "gl_draw.h"
#include "net.h"
#include "netui.h"
#include "session.h"
#include "sound.h"
#include "ui.h"

#define READ_MAX_PER_FRAME (64 * 1024)
#define RETRY_MS           1000
#define WAIT_MS            30000
#define HINT_MS            5000     /* how to leave the stream, shown at its start */

enum { BTN_UP = 1, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_OK, BTN_BACK, BTN_KB, BTN_L, BTN_R };

enum { ST_NO_NETWORK, ST_SEARCH, ST_NONE, ST_LIST, ST_ADDRESS, ST_PROBE, ST_CONNECTING, ST_ERROR, ST_STREAM };

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;   /* everything below */
static session_t ses;
static ui_t ui;
static config_t cfg;
static char cfgPath[512];
static int state = ST_NO_NETWORK;
static int targetPort = PORT_CARDPUTER;
static bool linked, connecting;
static uint32_t retryAt, streamSince;
static uint8_t rxbuf[8192];
static bool quitRequested;

/* network given by Java (UI thread) */
static char netName[64];
static uint8_t netIp[4];
static bool netKnown, netChanged;

/* buttons from Java: presses since the last frame, and held ones */
static unsigned pressed, held;

/* sound */
static bool audioAnchored;
static int audioLatencyMs;

static uint32_t now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000);
}

static bool down(int b)
{
    return (pressed & (1u << b)) != 0;
}

/* --- settings (GC-Explorer3's explorer3.conf format: last address, language) */

static void cfg_save(void)
{
    char out[128];
    int n = config_write(&cfg, out, sizeof(out));
    FILE *f = n > 0 ? fopen(cfgPath, "w") : NULL;
    if (f) {
        fwrite(out, 1, (size_t)n, f);
        fclose(f);
    }
}

static void cfg_follow(void)
{
    bool dirty = false;
    if (ses.state == SES_OPEN && (!cfg.has_ip || memcmp(cfg.ip, ui.target, 4) != 0)) {
        memcpy(cfg.ip, ui.target, 4);   /* an address that answered */
        cfg.has_ip = true;
        dirty = true;
    }
    if (ses.lang_known && ses.lang != cfg.lang) {
        cfg.lang = ses.lang;
        dirty = true;
    }
    if (dirty)
        cfg_save();
}

/* --- connection ----------------------------------------------------------------- */

static void nonce16(uint8_t out[16])
{
    uint32_t x = now_ms() ^ (uint32_t)rand() ^ 0x9E3779B9u;
    for (int i = 0; i < 16; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        out[i] = (uint8_t)(x >> 8);
    }
}

static bool begin_session(uint32_t now)
{
    char req[256], host[16];
    uint8_t nonce[16];
    addr_format(ui.target, host);
    nonce16(nonce);
    int n = session_begin(&ses, now, host, nonce, req, sizeof(req));
    if (n < 0 || link_write(req, n) < 0) {
        link_close();
        return false;
    }
    return true;
}

static void leave_stream(void)
{
    if (linked) {
        uint8_t bye[16];
        int n = session_close_frame(&ses, bye, sizeof(bye));
        if (n > 0)
            link_write(bye, n);
    }
    link_close();
    linked = connecting = false;
    ses.state = SES_IDLE;
}

static void start_search(uint32_t now)
{
    finder_start(ui.self, NULL);
    ui.since = now;
    state = ST_SEARCH;
}

static void connect_to(const uint8_t ip[4], int port, uint32_t now)
{
    memcpy(ui.target, ip, 4);
    targetPort = port;
    ui.since = now;
    ui.reason = NET_REASON_NONE;
    int r = link_start(ip, port);
    if (r < 0) {
        ui.reason = net_reason_connect(r);
        state = ST_ERROR;
        return;
    }
    state = ST_CONNECTING;
}

static void to_address(uint32_t now)
{
    entry_init(&ui.entry, ui.self, NULL, cfg.has_ip ? cfg.ip : NULL);
    ui.since = now;
    state = ST_ADDRESS;
}

static unsigned entry_keys_of(unsigned h)
{
    unsigned k = 0;
    if (h & (1u << BTN_LEFT))  k |= KEY_LEFT;
    if (h & (1u << BTN_RIGHT)) k |= KEY_RIGHT;
    if (h & (1u << BTN_UP))    k |= KEY_UP;
    if (h & (1u << BTN_DOWN))  k |= KEY_DOWN;
    if (h & (1u << BTN_L))     k |= KEY_L;
    if (h & (1u << BTN_R))     k |= KEY_R;
    return k;
}

/* --- states --------------------------------------------------------------------- */

static void step(uint32_t now)
{
    if (netChanged) {
        netChanged = false;
        snprintf(ui.net_name, sizeof(ui.net_name), "%s", netName);
        memcpy(ui.self, netIp, 4);
        if (!netKnown && state != ST_STREAM) {
            state = ST_NO_NETWORK;
        } else if (netKnown && state == ST_NO_NETWORK) {
            start_search(now);
        }
    }
    switch (state) {
    case ST_NO_NETWORK:
        if (down(BTN_BACK))
            quitRequested = true;
        break;

    case ST_SEARCH: {
        int st = finder_poll(ui.found, &ui.nfound);
        ui.progress = finder_progress();
        if (down(BTN_KB)) {
            to_address(now);
        } else if (down(BTN_BACK)) {
            quitRequested = true;
        } else if (st == FINDER_DONE) {
            if (ui.nfound == 1) {
                connect_to(ui.found[0].ip, ui.found[0].port, now);
            } else if (ui.nfound > 1) {
                ui.sel = 0;
                state = ST_LIST;
            } else {
                state = ST_NONE;
            }
        }
        break;
    }

    case ST_NONE:
        if (down(BTN_OK))
            start_search(now);
        else if (down(BTN_KB))
            to_address(now);
        else if (down(BTN_BACK))
            quitRequested = true;
        break;

    case ST_LIST:
        if (down(BTN_UP) && ui.sel > 0)
            ui.sel--;
        if (down(BTN_DOWN) && ui.sel < ui.nfound - 1 && ui.sel < 5)
            ui.sel++;
        if (down(BTN_OK))
            connect_to(ui.found[ui.sel].ip, ui.found[ui.sel].port, now);
        else if (down(BTN_KB))
            to_address(now);
        else if (down(BTN_BACK))
            quitRequested = true;
        break;

    case ST_ADDRESS:
        entry_keys(&ui.entry, entry_keys_of(held), now);
        if (down(BTN_OK)) {
            int st = addr_check(ui.entry.ip, ui.self, NULL);
            if (st == ADDR_OK || st == ADDR_OTHER_NET) {   /* other network: warned, not forbidden */
                memcpy(ui.target, ui.entry.ip, 4);
                finder_start(ui.self, ui.entry.ip);         /* which port: Cardputer or Android */
                ui.since = now;
                state = ST_PROBE;
            }
        } else if (down(BTN_BACK)) {
            start_search(now);
        }
        break;

    case ST_PROBE: {
        found_t f[FINDER_MAX];
        int n = 0;
        if (down(BTN_BACK)) {
            to_address(now);
        } else if (finder_poll(f, &n) == FINDER_DONE) {
            if (n > 0) {
                connect_to(f[0].ip, f[0].port, now);
            } else {
                ui.reason = NET_REASON_UNREACHABLE;
                state = ST_ERROR;
            }
        }
        break;
    }

    case ST_CONNECTING: {
        if (down(BTN_BACK)) {
            link_close();
            start_search(now);
            break;
        }
        int r = link_poll();
        if (r > 0) {
            linked = true;
            connecting = false;
            if (!begin_session(now)) {
                linked = false;
                ui.reason = NET_REASON_LOST;
                state = ST_ERROR;
                break;
            }
            streamSince = now;
            state = ST_STREAM;
        } else if (r < 0) {
            ui.reason = net_reason_connect(r);
            state = ST_ERROR;
        } else if ((int32_t)(now - ui.since) > WAIT_MS) {
            link_close();
            ui.reason = NET_REASON_UNREACHABLE;
            state = ST_ERROR;
        }
        break;
    }

    case ST_ERROR:
        if (down(BTN_OK))
            connect_to(ui.target, targetPort, now);
        else if (down(BTN_BACK))
            start_search(now);
        break;

    default:   /* ST_STREAM */
        if (down(BTN_BACK)) {
            leave_stream();
            start_search(now);
            break;
        }
        if (linked) {
            int total = 0;
            while (total < READ_MAX_PER_FRAME) {
                int n = link_read(rxbuf, sizeof(rxbuf));
                if (n == 0)
                    break;
                if (n < 0) {
                    session_lost(&ses, n == LINK_CLOSED ? SES_ERR_PEER : SES_ERR_IO);
                    break;
                }
                total += n;
                if (session_feed(&ses, rxbuf, (size_t)n, now) < 0)
                    break;
                if (ses.out_len) {   /* pong at once */
                    if (link_write(ses.out, (int)ses.out_len) < 0)
                        session_lost(&ses, SES_ERR_IO);
                    ses.out_len = 0;
                }
            }
            session_tick(&ses, now);
            if (ses.state == SES_CLOSED) {
                int reason = net_reason_session(ses.error);
                bool wasOpen = ses.frames > 0 || ses.texts > 0;
                link_close();
                linked = false;
                retryAt = now + RETRY_MS;
                if (!wasOpen && reason == NET_REASON_NOT_GAME) {
                    ui.reason = reason;   /* not the game: no point insisting */
                    state = ST_ERROR;
                }
            }
            break;
        }
        /* reconnection */
        if (!connecting) {
            if ((int32_t)(now - retryAt) >= 0 && link_start(ui.target, targetPort) == 0)
                connecting = true;
            break;
        }
        {
            int r = link_poll();
            if (r > 0) {
                connecting = false;
                linked = begin_session(now);
                if (!linked)
                    retryAt = now + RETRY_MS;
            } else if (r < 0) {
                connecting = false;
                retryAt = now + RETRY_MS;
            }
        }
        break;
    }
}

static void draw(uint32_t now)
{
    if (state == ST_STREAM) {
        if (panel_shown(&ses.panel)) {
            panel_draw(&ses.panel, now, ses.lang);
        } else {
            bool changed = false;
            for (size_t i = 0; i < sizeof(ses.dirty.bits) / sizeof(ses.dirty.bits[0]); i++)
                changed |= ses.dirty.bits[i] != 0;
            gl_screen(ses.screen, changed);
            memset(&ses.dirty, 0, sizeof(ses.dirty));
            banner_draw(&ses.banner, now);
        }
        netui_overlay(ses.lang, !linked || ses.state == SES_HANDSHAKE, ses.bad_panels > 0);
        if ((int32_t)(now - streamSince) < HINT_MS)
            ui_stream_hint(ses.lang, ui.ouya);
        return;
    }
    ui.lang = ses.lang;
    ui.now = now;
    switch (state) {
    case ST_NO_NETWORK: ui.screen = UI_NO_NETWORK; break;
    case ST_SEARCH:     ui.screen = UI_SEARCH; break;
    case ST_NONE:       ui.screen = UI_NONE; break;
    case ST_LIST:       ui.screen = UI_LIST; break;
    case ST_ADDRESS:    ui.screen = UI_ADDRESS; break;
    case ST_ERROR:      ui.screen = UI_ERROR; break;
    default:            ui.screen = UI_CONNECTING; break;   /* PROBE, CONNECTING */
    }
    ui_draw(&ui);
}

/* --- Java ---------------------------------------------------------------------- */

#define JNI_FN(ret, name) JNIEXPORT ret JNICALL Java_io_github_ryosaeba89_explorer3_Control_##name

/* Once per process: settings file, language before the crew's (0 French,
 * 1 English), buttons drawn as the OUYA's or the Xperia Play's. */
JNI_FN(void, start)(JNIEnv *env, jclass cls, jstring path, jint lang, jboolean ouya)
{
    (void)cls;
    static bool started;
    pthread_mutex_lock(&lock);
    if (!started) {
        started = true;
        const char *p = (*env)->GetStringUTFChars(env, path, NULL);
        snprintf(cfgPath, sizeof(cfgPath), "%s", p);
        (*env)->ReleaseStringUTFChars(env, path, p);
        config_defaults(&cfg);
        cfg.lang = lang;
        FILE *f = fopen(cfgPath, "r");
        if (f) {
            char text[512];
            size_t n = fread(text, 1, sizeof(text), f);
            fclose(f);
            config_parse(&cfg, text, n);
        }
        session_init(&ses);
        ses.lang = cfg.lang;   /* until the first L message */
        memset(&ui, 0, sizeof(ui));
        ui.ouya = ouya;
        srand(now_ms());
    }
    if (!ses.lang_known)
        ses.lang = lang;   /* the game's language, until the crew sends its own */
    quitRequested = false;
    pthread_mutex_unlock(&lock);
}

/* Android's connection: name and IPv4 address, empty when offline */
JNI_FN(void, setNetwork)(JNIEnv *env, jclass cls, jstring name, jstring ip)
{
    (void)cls;
    const char *n = (*env)->GetStringUTFChars(env, name, NULL);
    const char *i = (*env)->GetStringUTFChars(env, ip, NULL);
    uint8_t a[4] = {0};
    bool known = ip_parse(i, strlen(i), a);
    pthread_mutex_lock(&lock);
    if (known != netKnown || memcmp(a, netIp, 4) != 0 || strcmp(n, netName) != 0) {
        netKnown = known;
        memcpy(netIp, a, 4);
        snprintf(netName, sizeof(netName), "%s", n);
        netChanged = true;
    }
    pthread_mutex_unlock(&lock);
    (*env)->ReleaseStringUTFChars(env, name, n);
    (*env)->ReleaseStringUTFChars(env, ip, i);
}

JNI_FN(void, button)(JNIEnv *env, jclass cls, jint code, jboolean isDown)
{
    (void)env;
    (void)cls;
    if (code < BTN_UP || code > BTN_R)
        return;
    pthread_mutex_lock(&lock);
    if (isDown) {
        pressed |= 1u << code;
        held |= 1u << code;
    } else {
        held &= ~(1u << code);
    }
    pthread_mutex_unlock(&lock);
}

JNI_FN(void, surfaceCreated)(JNIEnv *env, jclass cls)
{
    (void)env;
    (void)cls;
    gl_init();
}

/* One frame (GL thread): returns false when the player asked to quit. */
JNI_FN(jboolean, frame)(JNIEnv *env, jclass cls, jint w, jint h)
{
    (void)env;
    (void)cls;
    uint32_t now = now_ms();
    pthread_mutex_lock(&lock);
    step(now);
    pressed = 0;
    cfg_follow();
    gl_begin(w, h);
    draw(now);
    gl_end();
    bool quit = quitRequested;
    quitRequested = false;
    pthread_mutex_unlock(&lock);
    return !quit;
}

/* The sound (audio thread): out = stereo frames at SOUND_RATE; latencyMs:
 * how long before a sample written now is heard. */
JNI_FN(void, mix)(JNIEnv *env, jclass cls, jshortArray out, jint latencyMs)
{
    (void)cls;
    jsize n = (*env)->GetArrayLength(env, out);
    jshort *dst = (*env)->GetShortArrayElements(env, out, NULL);
    pthread_mutex_lock(&lock);
    if (!audioAnchored || latencyMs != audioLatencyMs) {
        sound_anchor(&ses.sound, now_ms() + (uint32_t)latencyMs);   /* this block sounds then */
        audioAnchored = true;
        audioLatencyMs = latencyMs;
    }
    sound_render(&ses.sound, dst, n / 2);
    pthread_mutex_unlock(&lock);
    (*env)->ReleaseShortArrayElements(env, out, dst, 0);
}
