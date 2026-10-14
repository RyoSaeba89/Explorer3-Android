// Android: the Cardputer game (src/main.cpp, unchanged apart from the texts of
// tools/android-texts.sed and the read-only bridge.inc appended to it) runs on
// its own thread, with the PC simulator's headers (sim/include). Java (class
// Game) starts it, sends it the buttons and reads its picture.
#include <M5Cardputer.h>
#include <Preferences.h>
#include <jni.h>
#include <pthread.h>
#include <time.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <random>

#include "display.h"
#include "input.h"
#include "network.h"
#include "onscreen_keyboard.h"
#include "bridge.h"
#include "control_link.h"
#include "labels.h"

void setup();
void loop();

M5Cardputer_Class M5Cardputer;
M5_Class M5;

// ---------------------------------------------------------------- Arduino

namespace {

uint64_t nowMs() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

const uint64_t t0 = nowMs();
uint32_t randNext = 1;  // same generator as newlib (the ESP32's rand()), like the simulator
bool useHwRandom = true;
uint64_t newlibState = 1;

int newlibRand() {
    static uint32_t seeded = 0;
    if (seeded != randNext) {  // srand() called by randomSeed()
        newlibState = randNext;
        seeded = randNext;
    }
    newlibState = newlibState * 6364136223846793005ULL + 1;
    return (int)((newlibState >> 32) & 0x7fffffff);
}

}  // namespace

uint32_t millis() {
    return (uint32_t)(nowMs() - t0);
}

void delay(uint32_t ms) {
    timespec t = {(time_t)(ms / 1000), (long)(ms % 1000) * 1000000L};
    nanosleep(&t, nullptr);
}

uint32_t esp_random() {
    static std::random_device rd;
    return rd();
}

void randomSeed(unsigned long seed) {
    if (seed != 0) {
        randNext = seed;
        useHwRandom = false;
    }
}

long random(long howbig) {
    if (howbig <= 0) {
        return 0;
    }
    uint32_t v = useHwRandom ? esp_random() : (uint32_t)newlibRand();
    return v % howbig;
}

long random(long howsmall, long howbig) {
    if (howsmall >= howbig) {
        return howsmall;
    }
    return random(howbig - howsmall) + howsmall;
}

// ---------------------------------------------------------------- memory

namespace {
std::string settingsPath;
}

std::map<std::string, std::string> &Preferences::values() {
    static std::map<std::string, std::string> v;
    return v;
}

void Preferences::load(const std::string &path) {
    settingsPath = path;
    FILE *f = fopen(path.c_str(), "r");
    if (!f) {
        return;
    }
    char line[512];
    while (fgets(line, sizeof line, f)) {
        std::string s(line);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
        size_t eq = s.find('=');
        if (eq != std::string::npos) {
            values()[s.substr(0, eq)] = s.substr(eq + 1);
        }
    }
    fclose(f);
}

// Written to a temporary file then renamed, so a crash never leaves half a file
void Preferences::put(const char *k, const std::string &v) {
    values()[k] = v;
    if (settingsPath.empty()) {
        return;
    }
    std::string tmp = settingsPath + ".tmp";
    FILE *f = fopen(tmp.c_str(), "w");
    if (!f) {
        return;
    }
    for (const auto &e : values()) {
        fprintf(f, "%s=%s\n", e.first.c_str(), e.second.c_str());
    }
    fclose(f);
    rename(tmp.c_str(), settingsPath.c_str());
}

// ---------------------------------------------------------------- picture

namespace {
std::atomic<int> lastLanguage(0);  // read by Java
std::mutex pictureLock;
uint16_t picture[display::W * display::H];
int pictureCount = 0;
}  // namespace

namespace display {
void publish() {
    std::lock_guard<std::mutex> g(pictureLock);
    copy(picture);
    lastLanguage = bridge::language();
    osk::draw(picture);
    pictureCount++;
}
}  // namespace display

// ---------------------------------------------------------------- Java

namespace {

void *gameThread(void *) {
    setup();
    for (;;) {
        loop();
    }
    return nullptr;
}

}  // namespace

#define JNI_FN(ret, name) extern "C" JNIEXPORT ret JNICALL Java_io_github_ryosaeba89_explorer3_Game_##name

// Once per process: the game keeps running if the activity is recreated
JNI_FN(void, start)(JNIEnv *env, jclass, jstring settingsFile) {
    static bool started = false;
    if (started) {
        return;
    }
    started = true;
    const char *p = env->GetStringUTFChars(settingsFile, nullptr);
    Preferences::load(p);
    env->ReleaseStringUTFChars(settingsFile, p);
    pthread_t t;
    pthread_create(&t, nullptr, gameThread, nullptr);
    pthread_detach(t);
}

// Android's current connection: name (Wi-Fi name or "Ethernet") and IPv4
// address, both empty when not connected
JNI_FN(void, setNetwork)(JNIEnv *env, jclass, jstring name, jstring ip) {
    const char *n = env->GetStringUTFChars(name, nullptr);
    const char *i = env->GetStringUTFChars(ip, nullptr);
    network::set(n, i);
    env->ReleaseStringUTFChars(name, n);
    env->ReleaseStringUTFChars(ip, i);
}

// Mission control was chosen in the game's role menu and the network is up: true once
JNI_FN(jboolean, controlRequested)(JNIEnv *, jclass) {
    return control_link::requested();
}

// Back from the Control activity: the game goes back to its role menu
JNI_FN(void, controlClosed)(JNIEnv *, jclass) {
    control_link::finish();
}

// Game language, for mission control until the crew sends its own
JNI_FN(jint, language)(JNIEnv *, jclass) {
    return lastLanguage;
}

// The device used last (Game.STYLE_...): its texts from now on
JNI_FN(void, setStyle)(JNIEnv *, jclass, jint style) {
    labels::setStyle(style);
}

JNI_FN(void, press)(JNIEnv *, jclass, jint code) {
    input::push(code);
}

// A tap at (x, y) of the game's picture (240 x 135)
JNI_FN(void, tap)(JNIEnv *, jclass, jint x, jint y) {
    input::tap(x, y);
}

// Transparent buttons useful on the current screen (input::TouchButton bits)
JNI_FN(jint, touchButtons)(JNIEnv *, jclass) {
    return (jint)input::touchButtons();
}

// Copies the latest picture into dst (direct buffer of 240 x 135 x 2 bytes)
// if it is newer than "seen"; returns the picture's number.
JNI_FN(jint, picture)(JNIEnv *env, jclass, jobject dst, jint seen) {
    std::lock_guard<std::mutex> g(pictureLock);
    if (pictureCount != seen) {
        void *p = env->GetDirectBufferAddress(dst);
        if (p) {
            memcpy(p, picture, sizeof picture);
        }
    }
    return pictureCount;
}
