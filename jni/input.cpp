// Turns gamepad buttons, keyboard keys and taps into Cardputer key presses.
// Java queues codes from its own thread; the game thread reads them in
// Keyboard_Class::updateKeysState(), so the on-screen keyboard and the game's
// state (bridge.h) are only ever touched by the game thread.
//
// On-screen keyboard: it opens with the top face button (or a tap) where the
// screen expects a letter (Morse puzzles, quiz), and by itself for the
// start-up code and the coded keypad. It closes after an answer, when the
// screen changes, and with the back button.
//
// A tap becomes the presses that do the same with the gamepad (bridge::tap),
// given to the game one at a time so that each sees the previous one done.
#include "input.h"

#include <M5Cardputer.h>

#include <atomic>
#include <deque>
#include <mutex>

#include "bridge.h"
#include "display.h"
#include "onscreen_keyboard.h"
#include "role_menu.h"

namespace {

constexpr int TAP = 1 << 20;   // a tap in the codes from Java: TAP | y << 8 | x

std::mutex codeLock;
std::deque<int> codes;          // from Java
std::deque<KeysState> presses;  // for the game, game thread only
std::deque<int> tapCodes;       // what a tap stands for, applied one at a time
std::atomic<unsigned> buttonsNow(0);  // input::TouchButton bits, read by Java

uint32_t lastScreen = 0;
bool dismissed = false;  // keyboard closed by the player on this screen: no reopening by itself

// Same translation as the PC simulator: arrows are ; . , / and back is `
KeysState character(char c) {
    KeysState k;
    k.word.push_back(c);
    return k;
}

KeysState special(bool KeysState::*field) {
    KeysState k;
    k.*field = true;
    return k;
}

KeysState space() {
    KeysState k = special(&KeysState::space);
    k.word.push_back(' ');
    return k;
}

bool answers(bridge::Entry m) {
    return m == bridge::Entry::Letter || m == bridge::Entry::Quiz;
}

void closeByPlayer() {
    osk::close();
    dismissed = true;
}

// The open keyboard takes the buttons it uses; true if it did
bool keyboardButton(int code) {
    if (!osk::isOpen()) {
        return false;
    }
    bridge::Entry m = osk::mode();
    switch (code) {
        case input::UP: osk::move(0, -1); return true;
        case input::DOWN: osk::move(0, 1); return true;
        case input::LEFT: osk::move(-1, 0); return true;
        case input::RIGHT: osk::move(1, 0); return true;
        case input::CONFIRM: {
            int c = osk::selected();
            if (c == osk::ERASE) {
                if (answers(m)) {
                    closeByPlayer();  // nothing to erase: the arrow closes
                } else {
                    presses.push_back(special(&KeysState::del));
                }
            } else if (c == osk::OK) {
                presses.push_back(special(&KeysState::enter));  // closes when the code is accepted
            } else {
                presses.push_back(character((char)c));
                if (answers(m)) {
                    osk::close();  // one letter answers
                }
            }
            return true;
        }
        case input::SPACE:
            if (answers(m)) {
                return false;  // replays the Morse code while choosing
            }
            presses.push_back(special(&KeysState::del));
            return true;
        case input::BACK:
        case input::KEYBOARD:
            closeByPlayer();
            return true;
        default:
            return false;  // help, pause and plugged-in keyboard keys reach the game
    }
}

void click() {
    M5Cardputer.Speaker.tone(1200, 25, 1, true);  // the game's sfxClick()
}

// The role menu (after "Multiplayer") takes every button while it is open
bool roleButton(int code) {
    if (!role::isOpen()) {
        if ((code == input::CONFIRM || code == input::KEY_ENTER) && bridge::multiplayerSelected()) {
            role::open();
            click();
            return true;
        }
        return false;
    }
    switch (code) {
        case input::UP: role::move(-1); click(); break;
        case input::DOWN: role::move(1); click(); break;
        case input::CONFIRM:
        case input::KEY_ENTER:
            role::close();
            if (role::selected() == role::CREW) {
                presses.push_back(special(&KeysState::enter));  // the game goes on into Multiplayer
            } else {
                click();
                role::requestControl();
            }
            break;
        case input::BACK:
        case input::KEY_ESC:
            role::close();
            click();
            break;
        default:
            break;
    }
    return true;
}

void handle(int code);

// A tap: on the role menu or the on-screen keyboard, the choice or key
// touched; elsewhere what the game's screen makes of it (bridge::tap)
void handleTap(int x, int y) {
    if (role::isOpen()) {
        int i = role::choiceAt(x, y);
        if (i >= 0) {
            if (i != role::selected()) {
                role::move(1);
            }
            handle(input::CONFIRM);
        }
        return;
    }
    if (osk::isOpen()) {
        if (osk::selectAt(x, y)) {
            handle(input::CONFIRM);
            return;
        }
        if (osk::covers(x, y)) {
            return;
        }
    }
    bool codeKeyboard = osk::isOpen() && !answers(osk::mode());
    for (int code : bridge::tap(x, y)) {
        if (codeKeyboard && code == input::KEYBOARD) {
            continue;  // the keyboard of a code stays open: its keys are needed
        }
        tapCodes.push_back(code);
    }
}

// One code of a tap
void applyTap(int code) {
    if (code > ' ' && osk::isOpen() && answers(osk::mode())) {
        osk::close();  // an answer touched on the game's screen (quiz)
    }
    handle(code);
}

void handle(int code) {
    if (code & TAP) {
        handleTap(code & 0xff, (code >> 8) & 0xff);
        return;
    }
    if (roleButton(code) || keyboardButton(code)) {
        return;
    }
    switch (code) {
        case input::UP: presses.push_back(character(';')); break;
        case input::DOWN: presses.push_back(character('.')); break;
        case input::LEFT: presses.push_back(character(',')); break;
        case input::RIGHT: presses.push_back(character('/')); break;
        case input::CONFIRM:
        case input::KEY_ENTER:
            if (!bridge::waitingForControl()) {  // no game without mission control
                presses.push_back(special(&KeysState::enter));
            }
            break;
        case input::BACK:
        case input::KEY_ESC:
            presses.push_back(character('`'));
            break;
        case input::KEYBOARD:
            osk::open(bridge::entry());  // nothing opens where nothing is typed
            break;
        case input::SPACE:
            // Left face button: SPACE, or R (refresh) on the Wi-Fi list
            presses.push_back(bridge::wifiList() ? character('r') : space());
            break;
        case input::KEY_SPACE:
            presses.push_back(space());
            break;
        case input::HELP:
        case input::KEY_TAB:
            presses.push_back(special(&KeysState::tab));
            break;
        case input::KEY_DEL: presses.push_back(special(&KeysState::del)); break;
        case input::PAUSE:
            // The game pauses after Fn pressed 3 times within 800 ms each
            for (int i = 0; i < 3; i++) {
                presses.push_back(special(&KeysState::fn));
            }
            break;
        default:
            if (code > ' ' && code < 127) {
                presses.push_back(character((char)code));
            }
            break;
    }
}

// Keyboard follows the screen: closed on a new screen or when the screen
// no longer takes this kind of entry, opened by itself for codes
void followScreen() {
    uint32_t screen = bridge::screenId();
    if (screen != lastScreen) {
        lastScreen = screen;
        dismissed = false;
        osk::close();
        tapCodes.clear();  // a tap meant for the previous screen
        if (bridge::wifiList()) {
            // The game's Wi-Fi list (and its "create the Explorer3 network")
            // has no place on Android: back to the mode menu at once
            presses.push_back(character('`'));
        }
    }
    bridge::Entry e = bridge::entry();
    if (osk::isOpen() && osk::mode() != e) {
        osk::close();
    }
    if (!osk::isOpen() && !dismissed && (e == bridge::Entry::Code || e == bridge::Entry::Digits)) {
        osk::open(e);
    }
}

}  // namespace

namespace input {

void push(int code) {
    std::lock_guard<std::mutex> g(codeLock);
    codes.push_back(code);
}

void tap(int x, int y) {
    if (x >= 0 && x < display::W && y >= 0 && y < display::H) {
        push(TAP | y << 8 | x);
    }
}

unsigned touchButtons() {
    return buttonsNow;
}

}  // namespace input

// One press per loop turn, followed by a "released" turn, like the real keyboard
void Keyboard_Class::updateKeysState() {
    followScreen();
    std::deque<int> todo;
    {
        std::lock_guard<std::mutex> g(codeLock);
        todo.swap(codes);
    }
    for (int code : todo) {
        handle(code);
    }
    // The next code of a tap once the game has taken the previous one
    if (presses.empty() && !_release && !tapCodes.empty()) {
        int code = tapCodes.front();
        tapCodes.pop_front();
        applyTap(code);
    }
    unsigned b = bridge::touchButtons();
    if (role::isOpen()) {
        b = (b & ~input::TB_QUIT) | input::TB_BACK;
    }
    if (osk::isOpen()) {
        b |= input::TB_BACK;
    }
    buttonsNow = b;
    if (_release) {
        _release = false;
        _change = true;
        _pressed = false;
        _state = KeysState();
    } else if (!presses.empty()) {
        _state = presses.front();
        presses.pop_front();
        _change = _pressed = true;
        _release = true;
    } else {
        _change = _pressed = false;
    }
}
