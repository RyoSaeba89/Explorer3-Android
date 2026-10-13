
// ---------------------------------------------------------------- Android
// Appended to the copy of main.cpp by tools/build.sh (see jni/bridge.h).
// Only reads the game's state.
#include "bridge.h"
#include "input.h"

#include <cstdlib>

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

// ---- Touch. Where things are comes from the game's drawing functions,
// named in each comment; tools/build.sh stops if one of them has moved.

namespace {

bool inside(int x, int y, int left, int top, int w, int h) {
    return x >= left && x < left + w && y >= top && y < top + h;
}

// Entry of drawChoices(…, count, sel, top, h, gap) under (x, y), or -1
int choiceAt(int x, int y, int count, int top, int h, int gap) {
    for (int i = 0; i < count; i++) {
        if (inside(x, y, 16, top + i * (h + gap), W - 32, h)) {
            return i;
        }
    }
    return -1;
}

// Cell of a grid (left, top, cell size, cols x rows) under (x, y): false if none
bool cellAt(int x, int y, int left, int top, int cell, int cols, int rows, int &col, int &row) {
    if (x < left || y < top) {
        return false;
    }
    col = (x - left) / cell;
    row = (y - top) / cell;
    return col < cols && row < rows;
}

void repeat(std::vector<int> &out, int code, int n) {
    for (int i = 0; i < n; i++) {
        out.push_back(code);
    }
}

void tapPuzzle(std::vector<int> &out, int x, int y) {
    if (helpOpen) {
        out.push_back(input::HELP);  // any key closes the Morse code
        return;
    }
    int col, row;
    switch (puzzleKind()) {
        case Pz::Lamp:
        case Pz::Sound: {
            // drawPuzzleMorse: drawLamp / drawSpeaker(186, 74), 64 x 64 (none
            // in drawPuzzleSoundMulti); elsewhere the keyboard to answer
            bool device = puzzleKind() == Pz::Lamp || !multiMode;
            out.push_back(device && inside(x, y, 186 - 32, 74 - 32, 64, 64) ? input::SPACE : input::KEYBOARD);
            break;
        }
        case Pz::Quiz:
            // drawPuzzleQuiz: the 4 answers, 114 x 21
            for (int i = 0; i < 4; i++) {
                if (inside(x, y, 4 + (i % 2) * 118, 75 + (i / 2) * 23, 114, 21)) {
                    out.push_back(quizLabel[i]);
                }
            }
            break;
        case Pz::Maze:
            // drawPuzzleMaze: cells of 17 from (4, 34); a cell next to the astronaut
            if (!mazeDone && cellAt(x, y, 4, 34, 17, MAZE_W, MAZE_H, col, row)) {
                int dr = row - mazeR;
                int dc = col - mazeC;
                if (dr == -1 && dc == 0) out.push_back(input::UP);
                if (dr == 1 && dc == 0) out.push_back(input::DOWN);
                if (dr == 0 && dc == -1) out.push_back(input::LEFT);
                if (dr == 0 && dc == 1) out.push_back(input::RIGHT);
            }
            break;
        case Pz::Picross:
            // drawPuzzlePicross: cells of 17 from (35, 47); the cursor goes
            // there, then the cell is lit or put out (user's choice: one tap)
            if (cellAt(x, y, 35, 47, 17, 5, 5, col, row)) {
                repeat(out, col < curX ? input::LEFT : input::RIGHT, std::abs(col - curX));
                repeat(out, row < curY ? input::UP : input::DOWN, std::abs(row - curY));
                out.push_back(input::CONFIRM);
            }
            break;
    }
}

void tapComputer(std::vector<int> &out, int x, int y) {
    if (multiMode) {
        if (!keypadShown(millis())) {
            return;
        }
        // drawKeypad: keys 1 to 9, 46 x 32
        for (int k = 0; k < 9; k++) {
            if (inside(x, y, 96 + (k % 3) * 48, 18 + (k / 3) * 34, 46, 32)) {
                out.push_back('1' + k);
                return;
            }
        }
        out.push_back(input::KEYBOARD);  // erase and confirm are on the on-screen keyboard
    } else if (termVisible(millis()) >= TERM_COUNT) {
        out.push_back(input::KEYBOARD);
    }
}

}  // namespace

std::vector<int> tap(int x, int y) {
    std::vector<int> out;
    if (paused) {
        out.push_back(input::PAUSE);  // resumes
        return out;
    }
    int i;
    switch (state) {
        case St::Lang:
            // drawLang: drawChoices(…, 2, langSel, 32, 32, 8)
            i = choiceAt(x, y, 2, 32, 32, 8);
            if (i >= 0) {
                if (i != langSel) {
                    out.push_back(input::DOWN);
                }
                out.push_back(input::CONFIRM);
            }
            break;
        case St::Mode:
            // drawMode: drawChoices(…, 3, modeSel, 31, 28, 2); up and down go round
            i = choiceAt(x, y, 3, 31, 28, 2);
            if (i >= 0) {
                int down = (i - modeSel + 3) % 3;
                if (down == 2) {
                    out.push_back(input::UP);
                } else {
                    repeat(out, input::DOWN, down);
                }
                out.push_back(input::CONFIRM);
            }
            break;
        case St::Address:
        case St::Title:
        case St::Briefing:
        case St::Rules:
        case St::Solved:
        case St::Win:
        case St::GameOver:
            out.push_back(input::CONFIRM);  // "OK: continue": anywhere
            break;
        case St::Puzzle:
            tapPuzzle(out, x, y);
            break;
        case St::Computer:
            tapComputer(out, x, y);
            break;
        default:
            break;  // Wi-Fi screens (never shown on Android), connecting, lift-off
    }
    return out;
}

unsigned touchButtons() {
    if (paused) {
        return input::TB_PAUSE | input::TB_PAUSED | input::TB_QUIT;
    }
    unsigned b = 0;
    bool morse = state == St::Puzzle && (puzzleKind() == Pz::Lamp || puzzleKind() == Pz::Sound);
    if (canPause()) {
        b |= input::TB_PAUSE;
    }
    if (morse && !multiMode) {
        b |= input::TB_HELP;  // in multiplayer, mission control has the Morse code
    }
    if (morse && !helpOpen) {
        b |= input::TB_REPLAY;
    }
    switch (state) {
        case St::WifiList:
        case St::SsidEntry:
        case St::Password:
        case St::Connecting:
        case St::Address:
            b |= input::TB_BACK;
            break;
        case St::Lang:
        case St::Mode:
        case St::Title:
        case St::Win:
        case St::GameOver:
            b |= input::TB_QUIT;
            break;
        default:
            break;
    }
    if (helpOpen) {
        b |= input::TB_BACK;
    }
    return b;
}

}  // namespace bridge
