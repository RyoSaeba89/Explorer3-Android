# Changes to the copy of main.cpp for Android only (the Cardputer project is
# never modified). tools/build.sh checks that each one still applies.

# "Connect the screen": continuing is only offered once mission control is
# connected (user's request); input.cpp also ignores the button until then.
s/^\( *\)hint(tr(ADDR_CONTINUE), tx, 104, C_YELLOW);/\1if (mirror::clientCount() > 0) hint(tr(ADDR_CONTINUE), tx, 104, C_YELLOW);/

# tr() gives the text of the device used last: keyboard, Xperia Play, OUYA,
# PlayStation or Xbox gamepad, or the finger (padText, in bridge.inc; user's
# choice E1.1)
s/^const char \*tr(Tx &t) {$/const char *padText(Tx \&t, int l);\n\nconst char *tr(Tx \&t) {/
s/^    return t\[lang\];$/    return padText(t, lang);/
s/^    return multiMode ? multi\[lang\] : solo\[lang\];$/    return tr(multiMode ? multi : solo);/

# The Cardputer's own mission control is not built: the game's role menu
# opens the Control activity (jni/controle.h, bridge.inc; user's choice U1).
# Its core (the game's src/controle/) and its texts are left out of the copy.
/^extern "C" {$/,/^}$/d
/^#include "textes_controle.h"$/d
