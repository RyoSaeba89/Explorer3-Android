// Android: after "Multiplayer" in the game's mode menu, which role this
// device plays (user's choice, option 2): the crew (the game goes on as on
// the Cardputer) or mission control (Java opens the Control activity).
// Drawn over the game's screen in the game's own style, like the on-screen
// keyboard. Game thread only, except controlRequested().
#pragma once
#include <cstdint>

namespace role {

enum Choice { CREW = 0, CONTROL = 1 };

bool isOpen();
void open();   // the cursor starts on the crew
void close();
void move(int dy);
Choice selected();

// Touch: the choice under (x, y) of the picture, or -1
int choiceAt(int x, int y);

// Draws the menu over the whole picture (RGB565, display::W x display::H)
void draw(uint16_t *picture, int lang);

// Mission control was chosen: Java opens it (any thread; true once)
void requestControl();
bool controlRequested();

}  // namespace role
