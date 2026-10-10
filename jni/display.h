// The game's screen kept in memory (display_m5gfx.cpp) and the picture handed
// to Java (game.cpp).
#pragma once
#include <cstdint>

namespace display {

constexpr int W = 240;
constexpr int H = 135;

// Copies the screen as RGB565 in the CPU's byte order (as OpenGL wants it)
void copy(uint16_t *dst);

// Called by the game after each frame (mirror::unlockScreen): this picture,
// with the on-screen keyboard over it, becomes the one Java reads
void publish();

}  // namespace display
