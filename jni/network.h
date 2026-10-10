// The device's own network connection, set by Java (Android's Wi-Fi settings,
// or the OUYA's Ethernet): the game never chooses or creates a network on
// Android (user's choice). Any thread.
#pragma once
#include <string>

namespace network {

void set(const std::string &name, const std::string &ip);
std::string name();  // Wi-Fi name (or "Ethernet"); empty when not connected
std::string ip();    // "192.168.1.42"; empty when not connected

}  // namespace network
