// Between the game's mission control role (its role menu, then
// jni/controle.h) and Java: once the network is up the game asks for the
// Control activity (Screen.java polls Game.controlRequested()), and comes
// back to its role menu when that activity closes (Explorer3.onResume calls
// Game.controlClosed()). Any thread.
#pragma once

namespace control_link {

void request();     // game thread: open mission control
bool requested();   // Java: true once per request
void finish();      // Java: mission control closed
bool finished();    // game thread: true once per finish

}  // namespace control_link
