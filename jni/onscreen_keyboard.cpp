// On-screen keyboard, drawn with M5GFX like the game itself so that it is
// scaled and sharpened the same way. Rows of 18-unit cells; the arrow key
// erases (code) or closes (answers).
//   Letter  A..M / N..Z / [<-]            along the bottom
//   Quiz    the 4 answer letters [<-]     one row along the bottom, over the footer only
//   Code    A..M / N..Z / [<-] [OK]       along the top: the code boxes stay visible
//   Digits  1..9 [<-] [OK]                one row along the bottom, over the footer only
#include "onscreen_keyboard.h"

#include <M5GFX.h>

#include <vector>

#include "display.h"
#include "labels.h"

namespace osk {
namespace {

struct Key {
    int code;  // character, ERASE or OK
    int row, col, span;
};

constexpr int CELL_W = 18;
constexpr int CELL_H = 14;
constexpr int COLS = 13;
constexpr int LEFT = (display::W - COLS * CELL_W) / 2;
constexpr int HUD_H = 16;  // the game's top bar (oxygen, timer, code letters)

std::vector<Key> keys;
bridge::Entry current = bridge::Entry::None;
int rows = 0;
int cursor = 0;
int wantedCol = 0;  // column kept while moving up and down
lgfx::LGFX_Sprite sprite;

void letters(int fromRow) {
    for (int i = 0; i < 26; i++) {
        keys.push_back({'A' + i, fromRow + i / COLS, i % COLS, 1});
    }
}

void build(bridge::Entry m) {
    keys.clear();
    switch (m) {
        case bridge::Entry::Letter:
            letters(0);
            keys.push_back({ERASE, 2, 5, 3});
            rows = 3;
            break;
        case bridge::Entry::Quiz:
            for (int i = 0; i < 4; i++) {
                keys.push_back({bridge::quizLetter(i), 0, 2 + i * 2, 2});
            }
            keys.push_back({ERASE, 0, 10, 2});
            rows = 1;
            break;
        case bridge::Entry::Code:
            letters(0);
            keys.push_back({ERASE, 2, 3, 3});
            keys.push_back({OK, 2, 7, 3});
            rows = 3;
            break;
        case bridge::Entry::Digits:
            for (int i = 0; i < 9; i++) {
                keys.push_back({'1' + i, 0, i, 1});
            }
            keys.push_back({ERASE, 0, 9, 2});
            keys.push_back({OK, 0, 11, 2});
            rows = 1;
            break;
        default:
            rows = 0;
            break;
    }
}

int panelHeight() {
    return rows * CELL_H + 2;
}

int panelTop() {
    return current == bridge::Entry::Code ? HUD_H : display::H - panelHeight();
}

int indexOf(int code) {
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i].code == code) {
            return (int)i;
        }
    }
    return 0;
}

// Key of that row covering that column (the nearest one otherwise)
int keyAt(int row, int col) {
    int best = -1;
    int bestDist = 1000;
    for (size_t i = 0; i < keys.size(); i++) {
        const Key &k = keys[i];
        if (k.row != row) {
            continue;
        }
        int dist = col < k.col ? k.col - col : col >= k.col + k.span ? col - (k.col + k.span - 1) : 0;
        if (dist < bestDist) {
            best = (int)i;
            bestDist = dist;
        }
    }
    return best;
}

}  // namespace

bool isOpen() {
    return current != bridge::Entry::None;
}

bridge::Entry mode() {
    return current;
}

void open(bridge::Entry m) {
    build(m);
    current = keys.empty() ? bridge::Entry::None : m;
    if (isOpen()) {
        cursor = indexOf(ERASE);
        wantedCol = keys[cursor].col;
    }
}

void close() {
    current = bridge::Entry::None;
}

void move(int dx, int dy) {
    if (!isOpen()) {
        return;
    }
    const Key &k = keys[cursor];
    if (dx != 0) {
        // Left and right go round within the row
        int first = -1, last = -1;
        for (size_t i = 0; i < keys.size(); i++) {
            if (keys[i].row == k.row) {
                if (first < 0) first = (int)i;
                last = (int)i;
            }
        }
        int next = cursor + dx;
        if (next < first) next = last;
        if (next > last) next = first;
        cursor = next;
        wantedCol = keys[cursor].col;
    } else if (dy != 0 && rows > 1) {
        int row = (k.row + dy + rows) % rows;
        cursor = keyAt(row, wantedCol);
    }
}

int selected() {
    return isOpen() ? keys[cursor].code : OK;
}

bool covers(int x, int y) {
    return isOpen() && y >= panelTop() && y < panelTop() + panelHeight();
}

bool selectAt(int x, int y) {
    if (!covers(x, y)) {
        return false;
    }
    int top = panelTop() + (current == bridge::Entry::Code ? 0 : 2);  // as draw()
    for (size_t i = 0; i < keys.size(); i++) {
        const Key &k = keys[i];
        int kx = LEFT + k.col * CELL_W;
        int ky = top + k.row * CELL_H;
        if (x >= kx && x < kx + k.span * CELL_W && y >= ky && y < ky + CELL_H) {
            cursor = (int)i;
            wantedCol = k.col;
            return true;
        }
    }
    return false;
}

void draw(uint16_t *picture) {
    if (!isOpen()) {
        return;
    }
    if (sprite.width() == 0) {
        sprite.setColorDepth(16);
        sprite.createSprite(display::W, 3 * CELL_H + 2);
        sprite.setFont(&fonts::Font0);
        sprite.setTextDatum(middle_center);
    }
    const uint16_t back = sprite.color565(18, 22, 40);
    const uint16_t frame = sprite.color565(70, 90, 140);
    const uint16_t text = sprite.color565(235, 238, 245);
    const uint16_t yellow = sprite.color565(255, 225, 70);
    const int h = panelHeight();

    sprite.fillRect(0, 0, display::W, h, back);
    // Border line on the side facing the game
    sprite.drawFastHLine(0, current == bridge::Entry::Code ? h - 1 : 0, display::W, frame);
    int top = current == bridge::Entry::Code ? 0 : 2;
    for (size_t i = 0; i < keys.size(); i++) {
        const Key &k = keys[i];
        int x = LEFT + k.col * CELL_W + 1;
        int y = top + k.row * CELL_H;
        int w = k.span * CELL_W - 2;
        int kh = CELL_H - 2;
        // with the finger: no cursor, the keys are touched
        bool sel = (int)i == cursor && labels::style() != labels::TOUCH;
        uint16_t ink = sel ? back : text;
        if (sel) {
            sprite.fillRect(x, y, w, kh, yellow);
        } else {
            sprite.drawRect(x, y, w, kh, frame);
        }
        int cx = x + w / 2;
        int cy = y + kh / 2;
        if (k.code == ERASE) {
            sprite.fillTriangle(cx - 6, cy, cx - 2, cy - 4, cx - 2, cy + 4, ink);
            sprite.fillRect(cx - 2, cy - 1, 8, 3, ink);
        } else {
            char s[3] = {0};
            if (k.code == OK) {
                s[0] = 'O';
                s[1] = 'K';
            } else {
                s[0] = (char)k.code;
            }
            sprite.setTextColor(ink);
            sprite.drawString(s, cx + 1, cy + 1);
        }
    }

    // The sprite stores RGB565 high byte first
    const uint8_t *src = (const uint8_t *)sprite.getBuffer();
    uint16_t *dst = picture + panelTop() * display::W;
    for (int i = 0; i < display::W * h; i++, src += 2) {
        dst[i] = (uint16_t)(src[0] << 8 | src[1]);
    }
}

}  // namespace osk
