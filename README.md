# Explorer3-Android

**FR** — Le jeu [Escape Game Explorer 3](https://github.com/RyoSaeba89/Escape-Game-Cardputer) sur **Sony Ericsson Xperia Play** (Android 2.3) et **OUYA** (Android 4.1), dans **une seule APK**, jouable entièrement à la manette, ou au doigt sur tout écran tactile (téléphones compris) : mode **Solo**, et **Multijoueur** où l'appareil est soit l'**Équipage** (il joue, un navigateur sert de centre de contrôle), soit le **Centre de contrôle** (il sert d'écran face à un Cardputer ou à un autre appareil Android).

**EN** — The game [Escape Game Explorer 3](https://github.com/RyoSaeba89/Escape-Game-Cardputer) on the **Sony Ericsson Xperia Play** (Android 2.3) and the **OUYA** (Android 4.1), in **a single APK**, fully playable with the gamepad, or with the finger on any touch screen (phones included): **Solo** mode, and **Multiplayer** where the device is either the **Crew** (it plays, a web browser is mission control) or **Mission control** (it is the screen, facing a Cardputer or another Android device).

> **Version 3.0.0 (2026-10-13)** — le tactile, dans le jeu et le Centre de contrôle : on touche le jeu lui-même ; sans manette au démarrage, textes pour le doigt et boutons transparents ; adresse tapée sur le clavier chiffres d'Android. Le Centre de contrôle a son propre code. / touch, in the game and in Mission control: the game itself is touched; with no gamepad at start, texts for the finger and transparent buttons; the address is typed on Android's number keyboard. Mission control has its own code.
>
> **Version 2.0.0 (2026-10-12)** — pour le jeu v3.0.0 : le code de démarrage change à chaque partie ; au QCM, le clavier à l'écran propose les 4 lettres de la partie ; en centre de contrôle, les labyrinthes s'affichent avec leurs images. / for game v3.0.0: the start-up code changes with each game; at the quiz, the on-screen keyboard offers the game's 4 letters; as mission control, mazes show their pictures.
>
> **Version 1.0 (2026-10-10)** — pour le jeu v2.2.2. Essayée sur le Xperia Play (Solo, Équipage avec un navigateur, Centre de contrôle face à un Cardputer) ; **pas encore sur l'OUYA**. / for game v2.2.2. Tried on the Xperia Play (Solo, Crew with a browser, Mission control facing a Cardputer); **not yet on the OUYA**.

## Mode d'emploi / How to use

**FR**
1. Installer `explorer3.apk` : sur le Xperia Play, Paramètres → Applications → cocher « Sources inconnues », puis ouvrir l'APK avec un gestionnaire de fichiers ; sur l'OUYA, `adb install explorer3.apk` ou un gestionnaire de fichiers.
2. Lancer **Explorer 3** (lanceur Android, PlayStation Pocket du Xperia Play, ou menu des jeux de l'OUYA).
3. **Multijoueur** : connecter d'abord l'appareil au Wi-Fi dans les réglages d'Android (ou l'OUYA en Ethernet), puis choisir le rôle :
   - **Équipage** : l'écran « Connecter l'écran » donne l'adresse à ouvrir dans un navigateur, **avec le port** : `http://192.168.1.22:8080`. La partie ne démarre qu'une fois le centre de contrôle connecté.
   - **Centre de contrôle** : l'appareil cherche l'équipage tout seul sur le réseau (Cardputer ou Android) ; sinon, △ / Y pour taper son adresse ; au doigt, toucher cette indication puis l'adresse, qui se tape sur le clavier chiffres d'Android.

**EN**
1. Install `explorer3.apk`: on the Xperia Play, Settings → Applications → tick "Unknown sources", then open the APK with a file manager; on the OUYA, `adb install explorer3.apk` or a file manager.
2. Start **Explorer 3** (Android launcher, the Xperia Play's PlayStation Pocket, or the OUYA games menu).
3. **Multiplayer**: first connect the device to the Wi-Fi in Android's settings (or the OUYA by Ethernet), then choose the role:
   - **Crew**: the "Connect the screen" screen gives the address to open in a browser, **with the port**: `http://192.168.1.22:8080`. The game only starts once mission control is connected.
   - **Mission control**: the device looks for the crew on the network by itself (Cardputer or Android); otherwise, triangle / Y to type its address; with the finger, tap that hint then the address, typed on Android's number keyboard.

## Manette / Gamepad

| Rôle / Role | Xperia Play | OUYA |
|---|---|---|
| Déplacer / Move | croix / D-pad | croix / D-pad |
| Valider / Confirm | × | O |
| Retour / Back | ○ | A |
| Clavier à l'écran / On-screen keyboard | △ | Y |
| Écouter le Morse, effacer / Play the Morse, erase | □ | U |
| Aide (alphabet Morse) / Help (Morse code) | L, Select | L1 |
| Pause | R, Start | R1 |
| Quitter / Quit | Start + Select | Menu |

**FR** — Le clavier à l'écran s'ouvre avec △ / Y là où il faut taper une lettre, et se ferme tout seul après la réponse ; pour le code de démarrage, il s'ouvre seul en haut de l'écran. Le curseur part toujours sur ← (effacer).

**EN** — The on-screen keyboard opens with triangle / Y wherever a letter is expected and closes by itself after the answer; for the start-up code it opens by itself at the top of the screen. The cursor always starts on ← (erase).

## Tactile / Touch

**FR** — Sur tous les appareils, on touche le jeu lui-même : une ligne d'un menu, une réponse du QCM, une case du labyrinthe (voisine) ou du picross, les touches des claviers ; n'importe où pour « continuer » ; le texte d'une énigme de Morse ouvre le clavier à l'écran. La touche Retour d'Android revient en arrière. Si aucune manette n'est branchée au démarrage, les textes parlent du doigt et des boutons transparents apparaissent là où ils servent : ← retour, × quitter, ‖ pause / ▶ reprendre, ? alphabet Morse, ▶ revoir. Une manette BT ou USB branchée avant le démarrage remplace tout cela (textes × ○ △ □).

**EN** — On every device, the game itself is touched: a menu line, a quiz answer, a maze cell (next to the astronaut) or a picross cell, the keys of the keyboards; anywhere to "continue"; the text of a Morse puzzle opens the on-screen keyboard. Android's Back key goes back. If no gamepad is plugged in at start, the texts speak of the finger and transparent buttons show where they are useful: ← back, × quit, ‖ pause / ▶ resume, ? Morse code, ▶ again. A BT or USB gamepad plugged in before start replaces all this (× ○ △ □ texts).

## Documentation

| | |
|---|---|
| Documentation technique (français) | [docs/TECHNIQUE.md](docs/TECHNIQUE.md) |
| Technical documentation (English) | [docs/TECHNICAL.md](docs/TECHNICAL.md) |

**FR** — Pour compiler : ce dépôt à côté de [Escape-Game-Cardputer](https://github.com/RyoSaeba89/Escape-Game-Cardputer) (v3.0.0, après `pio run -e simulateur`), le NDK r15c et le SDK Android ; voir la documentation technique.

**EN** — To build: this repository next to [Escape-Game-Cardputer](https://github.com/RyoSaeba89/Escape-Game-Cardputer) (v3.0.0, after `pio run -e simulateur`), NDK r15c and the Android SDK; see the technical documentation.

```sh
tools/build.sh          # build/explorer3.apk
tools/build.sh --x86    # + x86, pour l'émulateur du PC / for the PC emulator
```
