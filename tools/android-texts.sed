# Game texts changed for Android only. tools/build.sh applies this file to a
# copy of src/textes.h (the Cardputer project itself is never modified), then
# turns the button placeholders into each device's buttons:
#   @OK@ confirm, @BACK@ back, @KB@ on-screen keyboard, @SQ@ left face button,
#   @L@ help, @R@ pause  (see tools/build.sh for the Xperia Play and OUYA names)
# The user's rule: the word "Cardputer" appears nowhere, in French or English.
# build.sh stops if a quoted text still contains it, or still names a
# Cardputer key (text added or changed in a newer version of the game).

# ---- Whole texts rewritten for the gamepad (before the general replacements)
/^constexpr Tx MODE_SOLO = /c\
constexpr Tx MODE_SOLO = {"Solo", "Solo"};
/^constexpr Tx MODE_SOLO_INFO = /c\
constexpr Tx MODE_SOLO_INFO = {"Tout le jeu sur cet appareil", "Everything on this device"};
/^constexpr Tx WIFI_REFRESH = /c\
constexpr Tx WIFI_REFRESH = {"@SQ@ : actualiser", "@SQ@: refresh"};
/^constexpr Tx WIFI_SCAN_FAILED = /c\
constexpr Tx WIFI_SCAN_FAILED = {"Recherche impossible : @SQ@ pour réessayer", "Search failed: @SQ@ to try again"};
/^constexpr Tx P1_FOOTER = /c\
constexpr Tx P1_FOOTER = {"@KB@ : répondre|@SQ@ : revoir|@L@ : Morse", "@KB@: answer|@SQ@: watch again|@L@: Morse"};
/^constexpr Tx P1_FOOTER_MULTI = /c\
constexpr Tx P1_FOOTER_MULTI = {"@KB@ : répondre|@SQ@ : revoir", "@KB@: answer|@SQ@: watch again"};
/^constexpr Tx P2_FOOTER = /c\
constexpr Tx P2_FOOTER = {"@KB@ : répondre", "@KB@: answer"};
/^constexpr Tx P3_FOOTER_MULTI = /c\
constexpr Tx P3_FOOTER_MULTI = {"@KB@ : répondre|@SQ@ : réécouter (contrôle)", "@KB@: answer|@SQ@: replay (mission control)"};
/^constexpr Tx P4_FOOTER = /c\
constexpr Tx P4_FOOTER = {"@KB@ : répondre|@SQ@ : réécouter|@L@ : Morse", "@KB@: answer|@SQ@: listen again|@L@: Morse"};
/^constexpr Tx MORSE_CLOSE = /c\
constexpr Tx MORSE_CLOSE = {"Un bouton : fermer", "Any button: close"};
/^constexpr Tx CODE_FOOTER = /c\
constexpr Tx CODE_FOOTER = {"@OK@ : taper|@SQ@ : effacer|@KB@ : clavier", "@OK@: type|@SQ@: erase|@KB@: keyboard"};
/^constexpr Tx KEYPAD_FOOTER = /c\
constexpr Tx KEYPAD_FOOTER = {"@OK@ : taper|@SQ@ : effacer|@KB@ : clavier", "@OK@: type|@SQ@: erase|@KB@: keyboard"};
/^constexpr Tx ADDR_FOOTER = /c\
constexpr Tx ADDR_FOOTER = {"@BACK@ : retour", "@BACK@: back"};
/^constexpr Tx PAUSE_RESUME = /c\
constexpr Tx PAUSE_RESUME = {"@R@ : reprendre", "@R@: resume"};

# ---- Cardputer key names in the other texts (only lines holding text)
/"/{
s/ENTRÉE \/ ENTER/@OK@/g
s/ENTRÉE/@OK@/g
s/ENTER/@OK@/g
s/ESPACE/@SQ@/g
s/SPACE/@SQ@/g
s/ESC/@BACK@/g
s/TAB/@L@/g
s/DEL/@SQ@/g
}
