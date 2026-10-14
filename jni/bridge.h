// Read-only view of the game's state for the Android input layer. Implemented
// in bridge.inc, which tools/build.sh appends to the copy of main.cpp: in the
// same translation unit, the game's internal variables are visible without
// changing a line of the game. Game thread only.
#pragma once
#include <cstdint>
#include <vector>

namespace bridge {

// What the current screen expects the player to type
enum class Entry {
    None,
    Letter,  // one letter answers (Morse puzzles)
    Quiz,    // one of the 4 letters shown answers (quizLetter)
    Code,    // start-up code: letters, erase, confirm
    Digits,  // coded keypad (multiplayer): 1 to 9, erase, confirm
};

Entry entry();

// Letter of the quiz answer i (0 to 3), drawn by the game for each game
char quizLetter(int i);

// Changes whenever the game moves to another screen
uint32_t screenId();

// Wi-Fi list (multiplayer): never kept on Android (input.cpp)
bool wifiList();

// "Connect the screen" while no mission control is connected: the game
// must not go on (user's request)
bool waitingForControl();

// Game language: 0 French, 1 English
int language();

// Touch (user's choice: the game itself is touched). The input codes
// (input.h) that a tap at (x, y) of the game's picture stands for, to apply
// one after the other; none where a tap does nothing. The on-screen keyboard
// is input.cpp's.
std::vector<int> tap(int x, int y);

// input::TouchButton bits useful on this screen, apart from what the
// on-screen keyboard adds (input.cpp)
unsigned touchButtons();

}  // namespace bridge
