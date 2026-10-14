# Mission control's page, copied from the Cardputer's diffusion.cpp into
# page.inc by tools/build.sh, changed for Android only:
s/ PROGMEM//
# Android forbids port 81 without root: the stream is on 8081
s/location.hostname+':81\//location.hostname+':8081\//
# Rule 4: the crew may play on a gamepad, the touch screen or a keyboard
s/Le chrono démarre quand l'équipage appuie sur ENTRÉE\./Le chrono démarre quand l'équipage lance la partie./
s/The countdown starts when the crew presses ENTER\./The countdown starts when the crew starts the game./
