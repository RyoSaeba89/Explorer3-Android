// Android: the Cardputer's NVS memory (language, best time, Wi-Fi) kept in a
// "key=value" file in the app's private folder, rewritten on every change.
// Same interface as the simulator's Preferences.h.
// The Wi-Fi keys ("ssid", "pass", "auto") always describe the network Android
// is already connected to: the game goes straight to it (user's choice).
#pragma once
#include <Arduino.h>

#include <map>

#include "network.h"

class Preferences {
public:
    bool begin(const char *, bool) { return true; }
    bool isKey(const char *k) { return values().count(k) > 0; }
    String getString(const char *k, const String &def) {
        std::string key(k);
        if (key == "ssid") return String(network::name());
        if (key == "pass") return String();
        return isKey(k) ? String(values()[k]) : def;
    }
    void putString(const char *k, const String &v) { put(k, v.c_str()); }
    uint32_t getUInt(const char *k, uint32_t def) { return isKey(k) ? (uint32_t)std::stoul(values()[k]) : def; }
    void putUInt(const char *k, uint32_t v) { put(k, std::to_string(v)); }
    uint8_t getUChar(const char *k, uint8_t def) { return isKey(k) ? (uint8_t)std::stoi(values()[k]) : def; }
    void putUChar(const char *k, uint8_t v) { put(k, std::to_string(v)); }
    bool getBool(const char *k, bool def) {
        if (std::string(k) == "auto") return true;
        return isKey(k) ? values()[k] == "1" : def;
    }
    void putBool(const char *k, bool v) { put(k, v ? "1" : "0"); }

    // game.cpp: read at start-up, before setup()
    static void load(const std::string &path);

private:
    static std::map<std::string, std::string> &values();
    static void put(const char *k, const std::string &v);
};
