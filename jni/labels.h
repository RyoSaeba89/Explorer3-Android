// Which device the player used last, set by Java (Device.java): its texts
// (tools/build.sh, padText() in bridge.inc), the on-screen keyboard's cursor
// and the transparent buttons follow it (user's choice E1.1). Any thread.
#pragma once

namespace labels {

// The order of the text versions in texts_table.inc (tools/build.sh) and of
// Game.STYLE_... in Java
enum Style { KEYBOARD, XPERIA, OUYA, PLAYSTATION, XBOX, TOUCH, COUNT };

Style style();
void setStyle(int style);

}  // namespace labels
