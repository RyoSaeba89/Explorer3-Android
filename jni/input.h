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
    // From 33 on: a printable character of a plugged-in keyboard
};

// Any thread: queues a button or key
void push(int code);

}  // namespace input
