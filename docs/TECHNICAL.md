# Explorer3-Android: technical documentation

*Version française : [TECHNIQUE.md](TECHNIQUE.md).*

## Contents

1. [Goal and status](#1-goal-and-status)
2. [Decisions](#2-decisions)
3. [Environment and build](#3-environment-and-build)
4. [Code layout](#4-code-layout)
5. [The game on Android](#5-the-game-on-android)
6. [Android texts and buttons](#6-android-texts-and-buttons)
7. [Gamepad, touch, on-screen keyboard, role choice](#7-gamepad-touch-on-screen-keyboard-role-choice)
8. [Sound](#8-sound)
9. [Multiplayer: the Crew](#9-multiplayer-the-crew)
10. [Multiplayer: Mission control](#10-multiplayer-mission-control)
11. [Sharp display](#11-sharp-display)
12. [Tests](#12-tests)
13. [Next steps](#13-next-steps)

## 1. Goal and status

A single APK for the Sony Ericsson **Xperia Play** (Android 2.3, API 10) and the **OUYA** (Android 4.1, API 16), played with the gamepad, and with the finger on any touch screen (phones included). The game is the Cardputer's ([Escape-Game-Cardputer](https://github.com/RyoSaeba89/Escape-Game-Cardputer) v3.0.0): `src/main.cpp` is **compiled for Android**, as its PC simulator already does. Mission control (`jni/control/`) does what the game's web page does; its core is shared with Explorer3-RaspberryPi. Android does not modify the game: it is read from its folder, and what changes for Android is applied to copies at build time.

**Version 1.0**: Solo, Multiplayer as the Crew and as Mission control. Tried on the Xperia Play (section 12); not yet on the OUYA.

**Version 3.0.0**: touch, in the game and in Mission control (section 7); Mission control has its own code, shared with Explorer3-RaspberryPi.

## 2. Decisions

Made by the user on 2026-10-10, then on 2026-10-13 for touch and Mission control's code. Not to be reopened without a new reason.

| Topic | Decision |
|---|---|
| Roles | Crew (the game runs on the device) and Mission control (the device is the screen) in the same APK, plus **Solo** mode. The role is chosen after "Multiplayer" in the game's menu. |
| Cardputer | No firmware change for Android: protocol v3.0.0 as is. |
| Network | No tethering: the device uses Android's Wi-Fi settings (or the OUYA's Ethernet); the game never searches, joins or creates a network. Mission control: automatic search for the crew, typed address as a fallback; home router or the Cardputer's Explorer3 network. |
| Crew ports | **8080** (page) and **8081** (stream): Android forbids 80 and 81 without root. |
| Technique | Cardputer game compiled natively (NDK r15c); Android side in Java; built by a script calling the SDK tools, no Gradle. |
| Display | Scaled to fit the whole screen, sharp and clean at any resolution (section 11). Mission-control pages faithful to the web page. No TV-margin setting. |
| Gamepad | Each maker's conventions (cross / O confirm, circle / A go back). Touchpads ignored. Quit: Start + Select, Menu on the OUYA. Other buttons: section 7. |
| Touch | All touch screens, in the game and in Mission control. The game itself is touched, no gamepad is drawn. Transparent virtual buttons (back, quit, pause, Morse, again), only where they are useful, in the black bands when the screen is wider than the picture, otherwise over its corners. |
| Gamepad detection | Once, at start. Xperia Play and OUYA: always a gamepad. Another device with a BT or USB gamepad: the Xperia Play's texts (× ○ △ □), no virtual buttons. No gamepad: texts for the finger and virtual buttons. |
| On-screen keyboard | Navigated with the D-pad; the cursor starts on the erase key. |
| Texts | The word "Cardputer" appears nowhere, in French or English; "Cardputer seul" becomes "Solo"; Cardputer keys are replaced by the device's buttons (section 6). |
| Home button | The game keeps running, as on the Cardputer (sound silenced while the app is in the background). |
| Mission control | Its code is in this repository (core shared with Explorer3-RaspberryPi), depending on no other project; the address is typed on Android's number keyboard, already filled in. |
| Identity | `io.github.ryosaeba89.explorer3`, "Explorer 3", icons cut from the title screen (`tools/icons.py`). |
| Publishing | Public repository, MIT licence, documentation in French and English, code and comments in English; commit after a test on the device; APK signed with a dedicated release key kept outside the repository. |

## 3. Environment and build

Next to this repository:

- **Escape-Game-Cardputer** v3.0.0 (`GAME_DIR=`, else `../Escape-Game-Cardputer`), with its simulator's M5GFX library, downloaded by `pio run -e simulateur` (`.pio/libdeps/simulateur/M5GFX`, version 0.2.32);

and:

- **NDK r15c** (`~/Android/android-ndk-r15c` or `NDK_R15C=`). It is the last one that can still target Android 2.3: it refuses by default, and `NDK_MIN_PLATFORM_LEVEL=9` forces its `android-9` platform, still shipped. r16b only goes down to API 14;
- the Android SDK: build-tools 34.0.0, platform `android-16`;
- a JDK (tried with 21); Python and Pillow only to remake the icons.

```sh
tools/build.sh          # build/explorer3.apk, ARM (armeabi-v7a)
tools/build.sh --x86    # adds x86 for the PC emulator
```

Script steps:

1. **Game copies**, one per set of buttons (`$NDK_OUT/game-xperia`, `game-ouya`, `game-touch`): `main.cpp` with `tools/android-game.sed`, then `jni/bridge.inc` appended; `textes.h` with `tools/android-texts.sed`, then the device's buttons (touch copy: `tools/touch-texts.sed`); `mots.h` (start-up codes and the data of their puzzles) and `dessins.h` (pictures of the maze cells) as they are; `diffusion.h` without `explorer3.local`; `page.inc`, mission control's page extracted from `diffusion.cpp` with `tools/android-page.sed`. The script **stops** if a displayed text still contains "Cardputer", a Cardputer key name or a placeholder, if a change no longer applies, or if something touch aims at has moved in the game's drawing (new game version).
2. **Mission control data**, taken from the game so that it always matches it (`$NDK_OUT/control`): `page_texts.inc`, the texts of the page's `TX` object (`tools/page-texts.awk`), rule 4 saying "starts the game" (the crew may be a Cardputer, an Android device or a Pi); `game_tables.inc`, the `MORSE`, `ASTRO` and `SYMBOLS` tables of `main.cpp`, as they are. The script stops if one of them changed shape.
3. **`ndk-build`**: four libraries, `libexplorer3_xperia.so`, `libexplorer3_ouya.so`, `libexplorer3_touch.so` (the game, each with its buttons) and `libexplorer3_control.so` (mission control). Output in a short folder (`NDK_OUT`, `~/Android/o` by default) because Windows limits path length.
4. `aapt2`, `javac` then `d8 --min-api 10`, `classes.dex` and `lib/` added, `zipalign`.
5. `apksigner` with `--min-sdk-version 10`, which adds the **v1 (JAR, SHA-1) signature** without which Android 2.3 refuses the APK. Key: `KEYSTORE=` and `KEYSTORE_PASS=` (a file holding the password); the SDK debug key by default. Releases are signed with the release key, kept outside the repository.

Game build settings, all outside the game: `-std=gnu++14` (the libc++ of r15c lacks `std::invoke_result`, M5GFX has a C++14 fallback); `-include cstdlib -include cmath` (M5GFX omissions); M5GFX platform `LGFX_LINUX_FB`; `-fvisibility=hidden`, separate sections and `--gc-sections`, without which M5GFX's Chinese, Korean and Japanese fonts make each library 45 MB (about 0.65 MB this way). Every system function imported exists in Android 2.3.

**Beware**: d8 (build-tools 34) fails on inner or anonymous Java classes compiled by JDK 21. The Java code therefore has none (the activities implement `Runnable` and `Handler.Callback`).

## 4. Code layout

| File | Role |
|---|---|
| `jni/game.cpp` | Game thread (`setup()` then `loop()`), Arduino functions, memory (`settings.txt`), published picture, the game's JNI functions |
| `jni/display_m5gfx.cpp` | In-memory screen (240 x 135); replaces the "desktop" part of `M5GFX.cpp` (SDL) |
| `jni/include/M5Cardputer.h`, `Preferences.h` | Replace the simulator's: real speaker, memory in a file, Android's network |
| `jni/audio.cpp` | The Cardputer's speaker (section 8) |
| `jni/input.cpp`, `input.h` | Buttons, keys and taps turned into Cardputer key presses |
| `jni/onscreen_keyboard.cpp` | On-screen keyboard |
| `jni/role_menu.cpp` | Role choice after "Multiplayer" |
| `jni/bridge.h`, `bridge.inc` | Reads the game's state (appended to the copy of `main.cpp`); what a tap aims at, the useful virtual buttons |
| `jni/mirror_android.cpp`, `sha1.h`, `network.h` | `mirror::`: the Crew's server (page and stream) and Android's network |
| `jni/control/control.c` | Mission control: main loop, settings (`control.conf`) |
| `jni/control/gl_draw.c` | OpenGL ES 2: `draw.h` and the copy of the crew's screen |
| `jni/control/net.c` | Connection to the crew and search on the network |
| `jni/control/ui.c` | Mission control's screens before the stream (search, list, address, connecting, error), notices over the stream |
| `jni/control/session.c`, `ws.c`, `msg.c`, `rows.c`, `clocksync.c`, `panel.c`, `banner.c`, `sound.c`, `addr.c`, `draw.c`, `text.c`, `texts.c` | Mission control's core, shared with Explorer3-RaspberryPi (same files): WebSocket protocol, messages, picture rows, clock, pages, banner, sound, address entry, drawing |
| `jni/control/gamedata.c`, `font_8x16.c` | The game's tables (`game_tables.inc`); libogc2's 8 x 16 CP437 font (zlib licence, in the file) |
| `java/.../Explorer3.java` | Game activity: gamepad, network, opening mission control |
| `java/.../Screen.java`, `Game.java`, `AudioOut.java` | The game's display, native methods and sound |
| `java/.../Control.java`, `ControlRenderer.java`, `ControlAudio.java` | Mission control's activity, display and sound |
| `java/.../Network.java`, `Device.java` | Android's network, device and gamepad detection |
| `java/.../TouchInput.java`, `TouchButtons.java`, `TouchTarget.java` | Taps on the screen, transparent virtual buttons |
| `tools/build.sh` | APK build |
| `tools/*.sed`, `tools/page-texts.awk` | Android changes to the copies of the game and its page; mission control's texts |
| `tools/icons.py` | Icons |

## 5. The game on Android

`main.cpp` is compiled with the headers of the game's PC simulator (`sim/include`), except `M5Cardputer.h` and `Preferences.h`, replaced by `jni/include`.

- **Screen**: `M5Cardputer.Display` gets an in-memory 240 x 135 `Panel_FrameBufferBase`. The Cardputer has a 135 x 240 panel that the game rotates (`setRotation(1)`); `offset_rotation = 3` cancels that rotation.
- **Picture**: the game calls `mirror::unlockScreen()` right after each frame; the picture is copied there (RGB565), the role choice and the on-screen keyboard are drawn over it, and a picture number is advanced. Java reads it on each refresh when the number has changed.
- **Thread**: the game runs on its own thread, started once per process. Quitting kills the process: the next game starts from the beginning. The Home button leaves the game running.
- **Memory**: language and best time in `settings.txt` (the app's private folder). The game's Wi-Fi keys (`ssid`, `pass`, `auto`) always describe the network Android is connected to.
- **Bridge** (`bridge.inc`): appended to the copy of `main.cpp`, it reads the game's internal variables (same translation unit) without changing anything: what the screen expects to be typed, screen changes, the Wi-Fi list, the address screen without mission control, "Multiplayer" selected, the language.

## 6. Android texts and buttons

`tools/android-texts.sed` replaces the Cardputer key names in `textes.h` with placeholders (`@OK@`, `@BACK@`, `@KB@`, `@SQ@`, `@L@`, `@R@`), rewrites a few texts for the gamepad, and removes "Cardputer". Each copy then gets its device's buttons:

| Placeholder | Role | Xperia Play | OUYA |
|---|---|---|---|
| `@OK@` | confirm (ENTER) | cross | O |
| `@BACK@` | back (ESC) | circle | A |
| `@KB@` | on-screen keyboard | triangle | Y |
| `@SQ@` | SPACE, erase | square | U |
| `@L@` | help (TAB) | L | L1 |
| `@R@` | pause (Fn x 3) | R | R1 |

The game's font (efontJA_12) has the × ○ □ △ signs. The APK also holds the touch library (`tools/touch-texts.sed`, section 7); `Game.java` loads the device's. Examples: "× : commencer", "△ : répondre | □ : revoir | L : Morse", "R : reprendre"; "Cardputer seul" becomes "Solo", its subtitle "Tout le jeu sur cet appareil".

`tools/android-game.sed` hides "× : continuer" on the address screen while no mission control is connected. `tools/android-page.sed` adjusts the page served by the Crew: stream on port 8081, "Connecting to the crew…", rule 4 naming the device's button.

## 7. Gamepad, touch, on-screen keyboard, role choice

| Role | Cardputer key | Xperia Play | OUYA | Android code |
|---|---|---|---|---|
| Move | `;` `.` `,` `/` | D-pad | D-pad | `DPAD_*` |
| Confirm | ENTER | cross | O | `DPAD_CENTER`, `BUTTON_A` |
| Back | ESC (`` ` ``) | circle | A | `BACK`, `BUTTON_B` |
| On-screen keyboard | — | triangle | Y | `BUTTON_Y` |
| Play the Morse | SPACE | square | U | `BUTTON_X` |
| Help | TAB | L, Select | L1 | `BUTTON_L1`, `BUTTON_SELECT` |
| Pause | Fn x 3 | R, Start | R1 | `BUTTON_R1`, `BUTTON_START` |
| Quit | — | Start + Select | Menu | `MENU` (OUYA only) |

Xperia Play keys read from the device's `/system/usr/keylayout/keypad-zeus.kl` and `keypad-game-zeus.kl`. Start and Select act on release, unless pressed together. Pause sends three Fn presses. The circle (Android's Back key) does not close the app.

**On-screen keyboard** (`onscreen_keyboard.cpp`), drawn with M5GFX and the game's font, according to what the screen expects:

- Morse puzzles: A–Z and ←, at the bottom; opened by triangle; a letter answers and closes it;
- quiz: the 4 answer letters, drawn by the game for each game (`bridge::quizLetter()`), and ←, over the footer only;
- start-up code: A–Z, ← (erase) and OK, **at the top**, opened by itself (the code boxes stay visible);
- coded keypad (multiplayer): 1–9, ← and OK, over the footer, opened by itself.

The cursor always starts on ←. The keyboard closes with circle or triangle, after an answer, and whenever the screen changes. Square erases (code) or replays the Morse (puzzles).

**Role choice** (`role_menu.cpp`): cross on "Multiplayer" opens, in the style of the game's language menu, "Crew" (cross goes on into the game) or "Mission control" (Java opens the `Control` activity in the game's language; circle, "Back to menu", returns).

**Touch**, on every device. `TouchInput.java` only keeps taps on the screen (`SOURCE_TOUCHSCREEN`: the Xperia Play's touchpads stay ignored), with one finger that does not slide. The tap is brought to the game's 240 x 135 coordinates (`Screen.toGame()`), then `bridge::tap()` turns it into gamepad presses, given to the game one at a time. The positions come from the game's drawing functions; `tools/build.sh` checks they have not moved.

| Screen | Tap |
|---|---|
| Menus (language, mode, role choice) | a line chooses it |
| "× : continue" screens, title, end of game, pause | anywhere |
| Morse | the lamp or the speaker plays the signal again; elsewhere, the on-screen keyboard opens |
| Quiz | an answer |
| Maze | a cell next to the astronaut |
| Picross | a cell: it lights up or goes out |
| Start-up code, coded keypad, on-screen keyboard | the keys |

Android's Back key stays "back". With no gamepad at start (`Device.TOUCH`), the `libexplorer3_touch.so` library shows the texts for the finger ("Tap: start", "▶: watch again", "?: Morse", "←: back") and `TouchButtons.java` draws transparent buttons, only those useful on the current screen (`bridge::touchButtons()`):

| Button | Place | When |
|---|---|---|
| ← back | top left | Wi-Fi and address screens, Morse code open |
| × quit | bottom left | language, mode, title, end of game, pause |
| ‖ pause / ▶ resume | top right | when the game can pause |
| ? Morse code | middle right | Morse puzzles, in Solo |
| ▶ again | bottom right | Morse puzzles |

They sit in the black bands beside the picture when the screen is wider than it (at least 44 dp), otherwise over the picture's corners, more transparent. A button lights up under the finger.

## 8. Sound

**Game**: `jni/audio.cpp` replaces the simulator's silent speaker with M5Unified's as the game uses it: 8 channels, one sound at a time per channel, sine notes (M5Unified's default tone), looped `playRaw` for the rumble, master and channel volumes. `AudioOut.java` ("sound" thread) asks for 22 050 Hz mono blocks and writes them to an `AudioTrack`. In multiplayer, as on the Cardputer, sounds go to mission control instead of the speaker.

**Mission control**: the synthesis of `sound.c` (like the web page) at 48 kHz stereo, written by `ControlAudio.java`; the first sample is anchored at "now + the `AudioTrack` latency".

Both are silenced while the app is in the background.

## 9. Multiplayer: the Crew

Java gives the game Android's network (`Network.java`: Wi-Fi first, with its name; otherwise a wired interface), every 2 s. The game goes straight from "Multiplayer" to "Connect the screen": its Wi-Fi list never shows (the bridge leaves it at once), nor "Create the Explorer3 network".

`jni/mirror_android.cpp` is `diffusion.cpp`'s server in POSIX sockets, on one thread:

- `http://<address>:8080/`: the page (`page.inc`) and `/sym.js`;
- `ws://<address>:8081/`: WebSocket handshake, compressed screen rows (25 pictures/s at most, only changed rows, the whole screen on each new connection except the banner), messages `P` (clock, 250 ms), `T` `S` `R` (sound), `K` (pages), `L` (language), `B` (banner), `M` (maze theme, before the page and on each connection); ping every 5 s, disconnection after 2 unanswered; 5 browsers at most.

The address screen shows `address:8080` (text and QR code). The game cannot go on while no mission control is connected.

## 10. Multiplayer: Mission control

`libexplorer3_control.so` = the core shared with Explorer3-RaspberryPi (WebSocket protocol, session, clock, picture rows, pages, banner, maze theme of message `M`, sound, address entry) + the Android part (`control.c`, `gl_draw.c`, `net.c`, `ui.c`):

- **Search** (`net.c`, on its own thread): the 254 addresses of the device's /24, ports **81** (Cardputer) and **8081** (Android crew), in batches of 96 connections of 1.5 s; each open port is confirmed by `GET /sym.js` on 80 or 8080, with `Host` = the address (on its Explorer3 network the Cardputer redirects any other name to its page). One crew found: connects at once; several: list; none: search again or type the address (triangle).
- **Address entry**: with the D-pad (L / R ±10), or with the finger on Android's number keyboard, already filled in; checked on both ports before connecting.
- **Touch**: the screens note what can be tapped while they are drawn (`ui_tap()`): the button hints, the crews of the list, the address. With no gamepad at start: transparent back and quit buttons, framed words instead of buttons in the hints.
- **Stream**: as in a web browser. Screen copy, pages, banner, sound; reconnects by itself after a break or 30 s of silence; circle goes back to the search (shown for 5 s at the start).
- Texts: those of the game's page (`page_texts.inc`, section 3); Android's own screens in `ui.c`, without the word "Cardputer". Buttons are drawn as the device's own.
- Settings: `control.conf`, lines `ip=` (the last crew that answered) and `lang=fr|en` (the crew's last language).

## 11. Sharp display

**Game** (`Screen.java`, OpenGL ES 2): the 240 x 135 picture is enlarged a whole number of times k **without smoothing** into an intermediate texture, then brought to its final size (the largest 16:9 that fits, centred) **with smoothing**: only the edges of the big pixels soften. On an OUYA in 1080p, k = 8 and the final size is exact. On the Xperia Play (854 x 480), k = 3, then 720 x 405 → 853 x 480.

**Mission control** (`gl_draw.c`): the 960 x 540 canvas of `draw.h` is scaled to fit the screen. Rectangles and text are drawn at the screen's resolution (8 x 16 font enlarged 4 times, then smoothed); the copy of the crew's screen uses the same method as the game.

**Web page** (on the computer): the Cardputer's, which already fills the window and keeps pixels sharp (`image-rendering: pixelated`).

## 12. Tests

**Xperia Play** (user, 2026-10-10): game, sound, labels, keyboard; multiplayer as the Crew with a browser; Mission control facing a v2.2.2 Cardputer (picross cursor included); Start + Select; PlayStation Pocket launcher.

**Emulator** (Android 5.1 x86 AVD, `tools/build.sh --x86`, keys through `adb shell input keyevent`, screenshots through `adb exec-out screencap`): the Solo game; on-screen keyboards; role choice; the Crew's server checked with a Python WebSocket client and headless Edge; Mission control facing a fake Cardputer in Python (with a small `/sym.js` server on port 80). In the emulator, addresses 10.0.2.1 to .7 all lead to the PC: the search finds the same fake Cardputer several times there.

**Version 3.0.0** (user, 2026-10-13): validated on the device (touch and the new Mission control).

**Not tried yet**: the OUYA; an Android Mission control facing an Android Crew.

## 13. Next steps

- Tests on the OUYA (planned) and with two Android devices together.
- Any new version of the game: check that `tools/build.sh` passes (it stops if a change no longer applies), then run the tests again.
