// Gamepad buttons and keyboard keys sent by Java (Game.java uses the same
// numbers), turned into Cardputer key presses on the game thread (input.cpp).
#pragma once

namespace input {

enum Code {
    // Gamepad, by role (see Explorer3.java for the physical buttons)
    UP = 1,
    DOWN,
    LEFT,
    RIGHT,
    CONFIRM,   // ENTER
    BACK,      // ESC (` key)
    KEYBOARD,  // opens / closes the on-screen keyboard
    SPACE,     // SPACE (play the Morse code); erases in a code keyboard
    HELP,      // TAB
    PAUSE,     // Fn pressed 3 times
    // Keys of a plugged-in keyboard, passed on as they are
    KEY_ENTER,
    KEY_ESC,
    KEY_DEL,
    KEY_TAB,
    KEY_SPACE,
    KEY_CTRL,  // Fn: the game master's pause, pressed 3 times
    // From 33 on: a printable character of a plugged-in keyboard
};

// Transparent touch buttons, shown when no gamepad was found at start
// (TouchButtons.java uses the same bits): the ones useful on this screen
enum TouchButton {
    TB_BACK = 1,
    TB_QUIT = 2,     // Java quits by itself
    TB_PAUSE = 4,
    TB_HELP = 8,     // Morse code (HELP)
    TB_REPLAY = 16,  // the Morse signal again (SPACE)
    TB_PAUSED = 32,  // not a button: the pause button shows "resume"
};

// Any thread: queues a button or key
void push(int code);

// Any thread: queues a tap at (x, y) of the game's picture (display::W x display::H)
void tap(int x, int y);

// Any thread: TouchButton bits for the current screen
unsigned touchButtons();

}  // namespace input
