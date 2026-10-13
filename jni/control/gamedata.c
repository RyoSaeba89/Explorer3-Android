/* See gamedata.h. On Android, mission control is a library apart from the
 * game's: tools/build.sh copies the game's tables (MORSE, ASTRO and SYMBOLS
 * of main.cpp, as they are) into game_tables.inc, so they are always those
 * of the game version built into the app. */
#include "gamedata.h"

#include "game_tables.inc"

_Static_assert(sizeof(MORSE) / sizeof(MORSE[0]) == 26, "MORSE: A to Z");
_Static_assert(sizeof(ASTRO) / sizeof(ASTRO[0]) == GAME_ASTRO_H, "ASTRO: rows");
_Static_assert(sizeof(SYMBOLS) / sizeof(SYMBOLS[0]) == GAME_SYMBOLS, "SYMBOLS: count");
_Static_assert(sizeof(SYMBOLS[0]) / sizeof(SYMBOLS[0][0]) == GAME_SYMBOL_SIZE, "SYMBOLS: rows");

const char *game_morse(int letter)
{
    return letter >= 0 && letter < 26 ? MORSE[letter] : "";
}

const char *game_astro_row(int row)
{
    return row >= 0 && row < GAME_ASTRO_H ? ASTRO[row] : "";
}

int game_symbol_pixel(int n, int row, int col)
{
    if (n < 0 || n >= GAME_SYMBOLS || row < 0 || row >= GAME_SYMBOL_SIZE || col < 0 || col >= GAME_SYMBOL_SIZE)
        return 0;
    return SYMBOLS[n][row][col] == 'X';
}
