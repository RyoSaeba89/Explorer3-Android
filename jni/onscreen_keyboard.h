// On-screen keyboard, for the gamepad and touch, drawn over the game's screen.
// Its keys depend on what the screen expects (bridge::Entry). Game thread only.
#pragma once
#include <cstdint>

#include "bridge.h"

namespace osk {

constexpr int ERASE = -1;  // erases, or closes the keyboard when there is nothing to erase
constexpr int OK = -2;

bool isOpen();
bridge::Entry mode();
void open(bridge::Entry mode);  // the cursor always starts on the arrow key (user's choice)
void close();
void move(int dx, int dy);
int selected();  // character, ERASE or OK

// Touch: the key under (x, y) of the picture becomes the selected one; false
// if there is none. covers: (x, y) is on the keyboard's panel.
bool selectAt(int x, int y);
bool covers(int x, int y);

// Draws the keyboard over the picture (RGB565, display::W x display::H)
void draw(uint16_t *picture);

}  // namespace osk
