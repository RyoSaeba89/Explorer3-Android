
// ---------------------------------------------------------------- Android
// Appended to the copy of main.cpp by tools/build.sh (see jni/bridge.h).
// Only reads the game's state.
#include "bridge.h"

namespace bridge {

Entry entry() {
    if (paused || helpOpen) {
        return Entry::None;
    }
    if (state == St::Puzzle) {
        switch (puzzleKind()) {
            case Pz::Lamp:
            case Pz::Sound: return Entry::Letter;
            case Pz::Quiz: return Entry::Quiz;
            default: return Entry::None;
        }
    }
    if (state == St::Computer) {
        if (multiMode) {
            return keypadShown(millis()) ? Entry::Digits : Entry::None;
        }
        return termVisible(millis()) >= TERM_COUNT ? Entry::Code : Entry::None;
    }
    return Entry::None;
}

char quizLetter(int i) {
    return quizLabel[i];
}

bool wifiList() {
    return state == St::WifiList;
}

bool waitingForControl() {
    return state == St::Address && mirror::clientCount() == 0;
}

bool multiplayerSelected() {
    return state == St::Mode && modeSel == 1 && !paused;
}

int language() {
    return lang;
}

uint32_t screenId() {
    return stateStart + (uint32_t)state * 7919u + (paused ? 1u : 0u);
}

}  // namespace bridge
