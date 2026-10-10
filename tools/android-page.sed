# Mission control's page, copied from the Cardputer's diffusion.cpp into
# page.inc by tools/build.sh, changed for Android only:
s/ PROGMEM//
# Android forbids port 81 without root: the stream is on 8081
s/location.hostname+':81\//location.hostname+':8081\//
# No "Cardputer" in any displayed text (user's rule)
s/Connexion au Cardputer…/Connexion à l’équipage…/
s/Connecting to the Cardputer…/Connecting to the crew…/
# Rule 4 names the crew's button: that of this device (placeholder turned into
# × or O by tools/build.sh)
s/appuie sur ENTRÉE\./appuie sur @OK@./
s/presses ENTER\./presses @OK@./
