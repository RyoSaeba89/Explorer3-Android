# Game texts for devices without a gamepad (user's choice: texts for the
# finger). tools/build.sh applies this file after android-texts.sed, for the
# touch copy of textes.h only: every text naming a button is rewritten whole
# (French and English differ). Touch words, in the hints' "key : action"
# form:
#   Toucher / Tap   a tap on the game's screen
#   ←               back (Android's back, or the transparent back button)
#   ▶               the Morse signal again (lamp, speaker, or the button)
#   ?               the Morse code (transparent button)
# Footers are 240 pixels wide at most (6 per character in the game's font).
# build.sh stops if a placeholder (@OK@...) is left over.

/^constexpr Tx LANG_FOOTER = /c\
constexpr Tx LANG_FOOTER = {"Toucher / Tap : choisir / select", "Toucher / Tap : choisir / select"};
/^constexpr Tx MODE_FOOTER = /c\
constexpr Tx MODE_FOOTER = {"Toucher : choisir", "Tap: choose"};
/^constexpr Tx WIFI_REFRESH = /c\
constexpr Tx WIFI_REFRESH = {"Actualiser", "Refresh"};
/^constexpr Tx WIFI_SCAN_FAILED = /c\
constexpr Tx WIFI_SCAN_FAILED = {"Recherche impossible", "Search failed"};
/^constexpr Tx WIFI_FOOTER = /c\
constexpr Tx WIFI_FOOTER = {"Toucher : choisir|← : retour", "Tap: choose|←: back"};
/^constexpr Tx TYPING_FOOTER = /c\
constexpr Tx TYPING_FOOTER = {"← : retour", "←: back"};
/^constexpr Tx CONNECT_FOOTER = /c\
constexpr Tx CONNECT_FOOTER = {"← : choisir un autre Wi-Fi", "←: choose another Wi-Fi"};
/^constexpr Tx ADDR_CONTINUE = /c\
constexpr Tx ADDR_CONTINUE = {"Toucher : continuer", "Tap: continue"};
/^constexpr Tx ADDR_FOOTER = /c\
constexpr Tx ADDR_FOOTER = {"← : retour", "←: back"};
/^constexpr Tx ADDR_FOOTER_AP = /c\
constexpr Tx ADDR_FOOTER_AP = {"← : retour", "←: back"};
/^constexpr Tx TITLE_START = /c\
constexpr Tx TITLE_START = {"Toucher : commencer", "Tap: start"};
/^constexpr Tx BRIEF_START = /c\
constexpr Tx BRIEF_START = {"Toucher : démarrer le chrono", "Tap: start the countdown"};
/^constexpr Tx BRIEF_NEXT = /c\
constexpr Tx BRIEF_NEXT = {"Toucher : continuer", "Tap: continue"};
/^constexpr Tx P1_FOOTER = /c\
constexpr Tx P1_FOOTER = {"Toucher : répondre|▶ : revoir|? : Morse", "Tap: answer|▶: watch again|?: Morse"};
/^constexpr Tx P1_FOOTER_MULTI = /c\
constexpr Tx P1_FOOTER_MULTI = {"Toucher : répondre|▶ : revoir", "Tap: answer|▶: watch again"};
/^constexpr Tx P2_FOOTER = /c\
constexpr Tx P2_FOOTER = {"Toucher une réponse", "Tap an answer"};
/^constexpr Tx P3_MOVE = /c\
constexpr Tx P3_MOVE = {"Toucher une case", "Tap a cell"};
/^constexpr Tx P3_LIGHT = /c\
constexpr Tx P3_LIGHT = {"pour l'allumer", "to light it up"};
/^constexpr Tx P3_FOOTER_MULTI = /c\
constexpr Tx P3_FOOTER_MULTI = {"Toucher : répondre|▶ : son (contrôle)", "Tap: answer|▶: replay (control)"};
/^constexpr Tx P4_FOOTER_MAZE = /{
N
c\
constexpr Tx P4_FOOTER_MAZE = {"Toucher une case voisine", "Tap a neighbouring cell"};
}
/^constexpr Tx P4_FOOTER = /c\
constexpr Tx P4_FOOTER = {"Toucher : répondre|▶ : écouter|? : Morse", "Tap: answer|▶: listen again|?: Morse"};
/^constexpr Tx MORSE_PLAYING = /c\
constexpr Tx MORSE_PLAYING = {"Signal en cours|? : alphabet Morse", "Signal playing|?: Morse code"};
/^constexpr Tx MORSE_CLOSE = /c\
constexpr Tx MORSE_CLOSE = {"Toucher : fermer", "Tap: close"};
/^constexpr Tx SOLVED_NEXT = /c\
constexpr Tx SOLVED_NEXT = {"Toucher : énigme suivante", "Tap: next puzzle"};
/^constexpr Tx CODE_FOOTER = /c\
constexpr Tx CODE_FOOTER = {"Toucher les touches", "Tap the keys"};
/^constexpr Tx KEYPAD_FOOTER = /c\
constexpr Tx KEYPAD_FOOTER = {"Toucher les touches", "Tap the keys"};
/^constexpr Tx WIN_AGAIN = /c\
constexpr Tx WIN_AGAIN = {"Toucher : rejouer", "Tap: play again"};
/^constexpr Tx LOST_AGAIN = /c\
constexpr Tx LOST_AGAIN = {"Toucher : nouvelle tentative", "Tap: try again"};
/^constexpr Tx PAUSE_RESUME = /c\
constexpr Tx PAUSE_RESUME = {"Toucher : reprendre", "Tap: resume"};

# Mission control's page (page.inc), rule 4: the crew starts the game
s/appuie sur @OK@\./touche l'écran./
s/presses @OK@\./taps the screen./
