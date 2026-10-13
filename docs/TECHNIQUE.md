# Explorer3-Android : documentation technique

*English version: [TECHNICAL.md](TECHNICAL.md).*

## Sommaire

1. [But et état](#1-but-et-état)
2. [Décisions](#2-décisions)
3. [Environnement et compilation](#3-environnement-et-compilation)
4. [Organisation du code](#4-organisation-du-code)
5. [Le jeu sur Android](#5-le-jeu-sur-android)
6. [Textes et boutons propres à Android](#6-textes-et-boutons-propres-à-android)
7. [Manette, tactile, clavier à l'écran, choix du rôle](#7-manette-tactile-clavier-à-lécran-choix-du-rôle)
8. [Son](#8-son)
9. [Multijoueur : l'Équipage](#9-multijoueur--léquipage)
10. [Multijoueur : le Centre de contrôle](#10-multijoueur--le-centre-de-contrôle)
11. [Affichage net](#11-affichage-net)
12. [Essais](#12-essais)
13. [La suite](#13-la-suite)

## 1. But et état

Une seule APK pour le Sony Ericsson **Xperia Play** (Android 2.3, API 10) et l'**OUYA** (Android 4.1, API 16), jouable à la manette, et au doigt sur tout écran tactile (téléphones compris). Le jeu est celui du Cardputer ([Escape-Game-Cardputer](https://github.com/RyoSaeba89/Escape-Game-Cardputer) v3.0.0) : `src/main.cpp` est **compilé pour Android**, comme le fait déjà son simulateur PC. Le Centre de contrôle (`jni/control/`) fait ce que fait la page web du jeu ; son cœur est commun avec Explorer3-RaspberryPi. Android ne modifie pas le jeu : il est lu depuis son dossier, et ce qui change pour Android est appliqué à des copies, au moment de la compilation.

**Version 1.0** : Solo, Multijoueur en Équipage et en Centre de contrôle. Essayée sur le Xperia Play (section 12) ; pas encore sur l'OUYA.

**Version 3.0.0** : le tactile, dans le jeu et le Centre de contrôle (section 7) ; le Centre de contrôle a son propre code, commun avec Explorer3-RaspberryPi.

## 2. Décisions

Prises par l'utilisateur le 2026-10-10, puis le 2026-10-13 pour le tactile et le code du Centre de contrôle. Elles ne sont pas à rediscuter sans raison nouvelle.

| Sujet | Décision |
|---|---|
| Rôles | Équipage (le jeu tourne sur l'appareil) et Centre de contrôle (l'appareil sert d'écran) dans la même APK, plus le mode **Solo**. Le rôle se choisit après « Multijoueur » dans le menu du jeu. |
| Cardputer | Aucune modification du firmware pour Android : protocole v3.0.0 tel quel. |
| Réseau | Pas de partage de connexion : l'appareil passe par les réglages Wi-Fi d'Android (ou l'Ethernet de l'OUYA) ; le jeu ne cherche, ne rejoint ni ne crée aucun réseau. Centre de contrôle : recherche automatique de l'équipage, saisie de l'adresse en secours ; réseau de la box ou réseau Explorer3 du Cardputer. |
| Ports de l'Équipage | **8080** (page) et **8081** (flux) : Android interdit 80 et 81 sans root. |
| Technique | Jeu du Cardputer compilé en natif (NDK r15c) ; partie Android en Java ; compilation par un script qui appelle les outils du SDK, sans Gradle. |
| Affichage | Mis à l'échelle de l'écran entier, net et propre quelle que soit la résolution (section 11). Pages du centre de contrôle fidèles à la page web. Pas de réglage de marge télé. |
| Manette | Conventions de chaque fabricant (× / O valident, ○ / A reviennent). Pavés tactiles ignorés. Quitter : Start + Select, Menu sur l'OUYA. Autres boutons : section 7. |
| Tactile | Tous les écrans tactiles, dans le jeu et le Centre de contrôle. On touche le jeu lui-même, aucune manette n'est dessinée. Boutons virtuels transparents (retour, quitter, pause, Morse, revoir), seulement là où ils servent, dans les bandes noires si l'écran est plus large que l'image, sinon sur ses coins. |
| Détection de la manette | Une fois, au démarrage. Xperia Play et OUYA : toujours une manette. Autre appareil avec une manette BT ou USB : textes du Xperia Play (× ○ △ □), pas de boutons virtuels. Sans manette : textes pour le doigt et boutons virtuels. |
| Clavier à l'écran | Parcouru à la croix ; le curseur part sur la touche effacer. |
| Textes | Le mot « Cardputer » n'apparaît nulle part, en français comme en anglais ; « Cardputer seul » devient « Solo » ; les touches du Cardputer sont remplacées par les boutons de l'appareil (section 6). |
| Bouton Accueil | Le jeu continue, comme sur le Cardputer (son coupé tant que l'application est en arrière-plan). |
| Centre de contrôle | Son code est dans ce dépôt (cœur commun avec Explorer3-RaspberryPi), sans dépendre d'un autre projet ; adresse tapée sur le clavier chiffres d'Android, déjà remplie. |
| Identité | `io.github.ryosaeba89.explorer3`, « Explorer 3 », icônes tirées de l'écran titre (`tools/icons.py`). |
| Publication | Dépôt public, licence MIT, documentation en français et en anglais, code et commentaires en anglais ; commit après un essai sur l'appareil ; APK signée avec une clé de release dédiée, gardée hors du dépôt. |

## 3. Environnement et compilation

Il faut, à côté de ce dépôt :

- **Escape-Game-Cardputer** v3.0.0 (`GAME_DIR=` sinon `../Escape-Game-Cardputer`), avec la bibliothèque M5GFX de son simulateur, téléchargée par `pio run -e simulateur` (`.pio/libdeps/simulateur/M5GFX`, version 0.2.32) ;

et :

- le **NDK r15c** (`~/Android/android-ndk-r15c` ou `NDK_R15C=`). C'est le dernier qui sait encore viser Android 2.3 : il le refuse par défaut, et `NDK_MIN_PLATFORM_LEVEL=9` le force sur sa plateforme `android-9`, toujours livrée. Le r16b ne descend qu'à l'API 14 ;
- le SDK Android : build-tools 34.0.0, plateforme `android-16` ;
- un JDK (essayé avec le 21) ; Python et Pillow seulement pour refaire les icônes.

```sh
tools/build.sh          # build/explorer3.apk, ARM (armeabi-v7a)
tools/build.sh --x86    # ajoute x86 pour l'émulateur du PC
```

Étapes du script :

1. **Copies du jeu**, une par jeu de boutons (`$NDK_OUT/game-xperia`, `game-ouya`, `game-touch`) : `main.cpp` avec `tools/android-game.sed` puis `jni/bridge.inc` ajouté à la fin ; `textes.h` avec `tools/android-texts.sed` puis les boutons de l'appareil (copie tactile : `tools/touch-texts.sed`) ; `mots.h` (codes de démarrage et données de leurs énigmes) et `dessins.h` (dessins des cases des labyrinthes) tels quels ; `diffusion.h` sans `explorer3.local` ; `page.inc`, la page du centre de contrôle extraite de `diffusion.cpp` avec `tools/android-page.sed`. Le script **s'arrête** si un texte affiché contient encore « Cardputer », un nom de touche du Cardputer ou un repère, si une modification ne s'applique plus, ou si un élément que le tactile vise a bougé dans le dessin du jeu (nouvelle version du jeu).
2. **Données du Centre de contrôle**, tirées du jeu pour toujours lui correspondre (`$NDK_OUT/control`) : `page_texts.inc`, les textes de l'objet `TX` de la page (`tools/page-texts.awk`), la règle 4 disant « lance la partie » (l'équipage peut être un Cardputer, un appareil Android ou un Pi) ; `game_tables.inc`, les tableaux `MORSE`, `ASTRO` et `SYMBOLS` de `main.cpp`, tels quels. Le script s'arrête si l'un d'eux a changé de forme.
3. **`ndk-build`** : quatre bibliothèques, `libexplorer3_xperia.so`, `libexplorer3_ouya.so`, `libexplorer3_touch.so` (le jeu, chacun avec ses boutons) et `libexplorer3_control.so` (le Centre de contrôle). Sortie dans un dossier court (`NDK_OUT`, `~/Android/o` par défaut), car Windows limite la longueur des chemins.
4. `aapt2`, `javac` puis `d8 --min-api 10`, ajout de `classes.dex` et `lib/`, `zipalign`.
5. `apksigner` avec `--min-sdk-version 10`, qui ajoute la **signature v1 (JAR, SHA-1)** sans laquelle Android 2.3 refuse l'APK. Clé : `KEYSTORE=` et `KEYSTORE_PASS=` (fichier contenant le mot de passe) ; par défaut la clé de débogage du SDK. Les releases sont signées avec la clé de release, gardée hors du dépôt.

Réglages de compilation du jeu, tous hors du jeu : `-std=gnu++14` (la libc++ du r15c n'a pas `std::invoke_result`, M5GFX a une variante C++14) ; `-include cstdlib -include cmath` (oublis de M5GFX) ; plateforme M5GFX `LGFX_LINUX_FB` ; `-fvisibility=hidden`, sections séparées et `--gc-sections`, sans quoi les polices chinoises, coréennes et japonaises de M5GFX font monter chaque bibliothèque à 45 Mo (environ 0,65 Mo ainsi). Toutes les fonctions système importées existent dans Android 2.3.

**Attention** : d8 (build-tools 34) plante sur les classes Java internes ou anonymes compilées par le JDK 21. Le code Java n'en a donc aucune (les activités implémentent `Runnable` et `Handler.Callback`).

## 4. Organisation du code

| Fichier | Rôle |
|---|---|
| `jni/game.cpp` | Fil du jeu (`setup()` puis `loop()`), fonctions Arduino, mémoire (`settings.txt`), image publiée, fonctions JNI du jeu |
| `jni/display_m5gfx.cpp` | Écran en mémoire (240 × 135) et partie « bureau » de `M5GFX.cpp` (SDL) remplacée |
| `jni/include/M5Cardputer.h`, `Preferences.h` | Remplacent ceux du simulateur : vrai haut-parleur, mémoire dans un fichier, réseau d'Android |
| `jni/audio.cpp` | Le haut-parleur du Cardputer (section 8) |
| `jni/input.cpp`, `input.h` | Boutons, touches et touchers traduits en appuis du clavier du Cardputer |
| `jni/onscreen_keyboard.cpp` | Clavier à l'écran |
| `jni/role_menu.cpp` | Choix du rôle après « Multijoueur » |
| `jni/bridge.h`, `bridge.inc` | Lecture de l'état du jeu (ajoutée à la copie de `main.cpp`) ; ce qu'un toucher vise, les boutons virtuels utiles |
| `jni/mirror_android.cpp`, `sha1.h`, `network.h` | `mirror::` : serveur de l'Équipage (page et flux) et réseau d'Android |
| `jni/control/control.c` | Centre de contrôle : boucle principale, réglages (`control.conf`) |
| `jni/control/gl_draw.c` | OpenGL ES 2 : `draw.h` et copie de l'écran de l'équipage |
| `jni/control/net.c` | Connexion à l'équipage et recherche sur le réseau |
| `jni/control/ui.c` | Écrans du Centre de contrôle avant le flux (recherche, liste, adresse, connexion, erreur), avis par-dessus le flux |
| `jni/control/session.c`, `ws.c`, `msg.c`, `rows.c`, `clocksync.c`, `panel.c`, `banner.c`, `sound.c`, `addr.c`, `draw.c`, `text.c`, `texts.c` | Cœur du Centre de contrôle, commun avec Explorer3-RaspberryPi (mêmes fichiers) : protocole WebSocket, messages, lignes d'image, horloge, pages, bandeau, son, saisie d'adresse, dessin |
| `jni/control/gamedata.c`, `font_8x16.c` | Tableaux du jeu (`game_tables.inc`) ; police 8 × 16 CP437 de libogc2 (licence zlib, dans le fichier) |
| `java/.../Explorer3.java` | Activité du jeu : manette, réseau, ouverture du Centre de contrôle |
| `java/.../Screen.java`, `Game.java`, `AudioOut.java` | Affichage, méthodes natives et son du jeu |
| `java/.../Control.java`, `ControlRenderer.java`, `ControlAudio.java` | Activité, affichage et son du Centre de contrôle |
| `java/.../Network.java`, `Device.java` | Réseau d'Android, détection de l'appareil et de la manette |
| `java/.../TouchInput.java`, `TouchButtons.java`, `TouchTarget.java` | Touchers de l'écran, boutons virtuels transparents |
| `tools/build.sh` | Construction de l'APK |
| `tools/*.sed`, `tools/page-texts.awk` | Modifications pour Android des copies du jeu et de sa page ; textes du Centre de contrôle |
| `tools/icons.py` | Icônes |

## 5. Le jeu sur Android

`main.cpp` est compilé avec les en-têtes du simulateur PC du jeu (`sim/include`), sauf `M5Cardputer.h` et `Preferences.h`, remplacés par `jni/include`.

- **Écran** : `M5Cardputer.Display` reçoit un panneau `Panel_FrameBufferBase` en mémoire de 240 × 135. Le Cardputer a un écran 135 × 240 que le jeu tourne (`setRotation(1)`) ; `offset_rotation = 3` annule cette rotation.
- **Image** : le jeu appelle `mirror::unlockScreen()` juste après chaque dessin ; l'image y est copiée (RGB565), le choix du rôle et le clavier à l'écran dessinés par-dessus, et un numéro d'image avancé. Java la lit à chaque rafraîchissement si le numéro a changé.
- **Fil** : le jeu tourne sur son propre fil, lancé une fois par processus. Quitter tue le processus : la partie suivante repart du début. Le bouton Accueil laisse le jeu tourner.
- **Mémoire** : langue et record dans `settings.txt` (dossier privé de l'application). Les clés Wi-Fi du jeu (`ssid`, `pass`, `auto`) décrivent toujours le réseau où Android est connecté.
- **Pont** (`bridge.inc`) : ajouté à la fin de la copie de `main.cpp`, il lit les variables internes du jeu (même unité de compilation) sans rien changer : ce que l'écran attend comme saisie, le changement d'écran, la liste Wi-Fi, l'écran d'adresse sans centre de contrôle, « Multijoueur » sélectionné, la langue.

## 6. Textes et boutons propres à Android

`tools/android-texts.sed` remplace dans `textes.h` les noms des touches du Cardputer par des repères (`@OK@`, `@BACK@`, `@KB@`, `@SQ@`, `@L@`, `@R@`), réécrit quelques textes pour la manette, et enlève « Cardputer ». Puis chaque copie reçoit les boutons de son appareil :

| Repère | Rôle | Xperia Play | OUYA |
|---|---|---|---|
| `@OK@` | valider (ENTRÉE) | × | O |
| `@BACK@` | retour (ESC) | ○ | A |
| `@KB@` | clavier à l'écran | △ | Y |
| `@SQ@` | ESPACE, effacer | □ | U |
| `@L@` | aide (TAB) | L | L1 |
| `@R@` | pause (Fn × 3) | R | R1 |

La police du jeu (efontJA_12) a les signes × ○ □ △. L'APK contient aussi la bibliothèque tactile (`tools/touch-texts.sed`, section 7) ; `Game.java` charge celle de l'appareil. Exemples : « × : commencer », « △ : répondre | □ : revoir | L : Morse », « R : reprendre » ; « Cardputer seul » devient « Solo », son sous-titre « Tout le jeu sur cet appareil ».

`tools/android-game.sed` masque « × : continuer » sur l'écran d'adresse tant qu'aucun centre de contrôle n'est connecté. `tools/android-page.sed` règle la page servie par l'Équipage : flux sur le port 8081, « Connexion à l'équipage… », règle 4 avec le bouton de l'appareil.

## 7. Manette, tactile, clavier à l'écran, choix du rôle

| Rôle | Touche du Cardputer | Xperia Play | OUYA | Code Android |
|---|---|---|---|---|
| Déplacer | `;` `.` `,` `/` | croix | croix | `DPAD_*` |
| Valider | ENTRÉE | × | O | `DPAD_CENTER`, `BUTTON_A` |
| Retour | ESC (`` ` ``) | ○ | A | `BACK`, `BUTTON_B` |
| Clavier à l'écran | — | △ | Y | `BUTTON_Y` |
| Écouter le Morse | ESPACE | □ | U | `BUTTON_X` |
| Aide | TAB | L, Select | L1 | `BUTTON_L1`, `BUTTON_SELECT` |
| Pause | Fn × 3 | R, Start | R1 | `BUTTON_R1`, `BUTTON_START` |
| Quitter | — | Start + Select | Menu | `MENU` (OUYA seulement) |

Touches du Xperia Play relevées dans `/system/usr/keylayout/keypad-zeus.kl` et `keypad-game-zeus.kl` de l'appareil. Start et Select agissent au relâchement, sauf s'ils ont été pressés ensemble. La pause envoie trois appuis sur Fn. ○ (la touche Retour d'Android) ne ferme pas l'application.

**Clavier à l'écran** (`onscreen_keyboard.cpp`), dessiné avec M5GFX et la police du jeu, selon ce que l'écran attend :

- énigmes de Morse : A–Z et ←, en bas ; ouvert par △ ; une lettre répond et le ferme ;
- QCM : les 4 lettres des réponses, tirées par le jeu à chaque partie (`bridge::quizLetter()`), et ←, sur le pied de page seulement ;
- code de démarrage : A–Z, ← (effacer) et OK, **en haut**, ouvert tout seul (les cases du code restent visibles) ;
- clavier codé (multijoueur) : 1–9, ← et OK, sur le pied de page, ouvert tout seul.

Le curseur part toujours sur ←. Le clavier se ferme avec ○ ou △, après une réponse, et à chaque changement d'écran. □ efface (code) ou rejoue le Morse (énigmes).

**Choix du rôle** (`role_menu.cpp`) : × sur « Multijoueur » ouvre, dans le style du menu de langue du jeu, « Équipage » (× poursuit dans le jeu) ou « Centre de contrôle » (Java ouvre l'activité `Control` dans la langue du jeu ; ○ « Retour au menu » y revient).

**Tactile**, sur tous les appareils. `TouchInput.java` ne garde que les touchers de l'écran (`SOURCE_TOUCHSCREEN` : les pavés tactiles du Xperia Play restent ignorés) et un seul doigt qui ne glisse pas. Le toucher est ramené aux coordonnées 240 × 135 du jeu (`Screen.toGame()`), puis `bridge::tap()` le change en appuis de la manette, donnés au jeu un par un. Les positions viennent des fonctions de dessin du jeu ; `tools/build.sh` vérifie qu'elles n'ont pas bougé.

| Écran | Toucher |
|---|---|
| Menus (langue, mode, choix du rôle) | une ligne la choisit |
| Écrans « × : continuer », titre, fin de partie, pause | n'importe où |
| Morse | la lampe ou le haut-parleur rejoue le signal ; ailleurs, le clavier à l'écran s'ouvre |
| QCM | une réponse |
| Labyrinthe | une case voisine de l'astronaute |
| Picross | une case : elle s'allume ou s'éteint |
| Code de démarrage, clavier codé, clavier à l'écran | les touches |

La touche Retour d'Android reste « retour ». Sans manette au démarrage (`Device.TOUCH`), la bibliothèque `libexplorer3_touch.so` affiche les textes pour le doigt (« Toucher : commencer », « ▶ : revoir », « ? : Morse », « ← : retour ») et `TouchButtons.java` dessine des boutons transparents, seulement ceux utiles à l'écran affiché (`bridge::touchButtons()`) :

| Bouton | Place | Quand |
|---|---|---|
| ← retour | en haut à gauche | écrans Wi-Fi et d'adresse, alphabet Morse ouvert |
| × quitter | en bas à gauche | langue, mode, titre, fin de partie, pause |
| ‖ pause / ▶ reprendre | en haut à droite | quand le jeu peut se mettre en pause |
| ? alphabet Morse | au milieu à droite | énigmes de Morse, en Solo |
| ▶ revoir | en bas à droite | énigmes de Morse |

Ils sont dans les bandes noires à côté de l'image quand l'écran est plus large qu'elle (au moins 44 dp), sinon sur les coins de l'image, plus transparents. Sous le doigt, un bouton s'éclaire.

## 8. Son

**Jeu** : `jni/audio.cpp` remplace le haut-parleur muet du simulateur par celui de M5Unified tel que le jeu l'utilise : 8 canaux, un son à la fois par canal, notes en sinus (le son par défaut de M5Unified), `playRaw` en boucle pour le grondement, volumes général et par canal. `AudioOut.java` (fil « sound ») demande des blocs à 22 050 Hz mono et les écrit dans un `AudioTrack`. En multijoueur, comme sur le Cardputer, les sons partent vers le centre de contrôle au lieu du haut-parleur.

**Centre de contrôle** : la synthèse de `sound.c` (comme la page web) à 48 kHz stéréo, écrite par `ControlAudio.java` ; le premier échantillon est ancré à « maintenant + latence de l'`AudioTrack` ».

Les deux sont coupés quand l'application est en arrière-plan.

## 9. Multijoueur : l'Équipage

Java donne au jeu le réseau d'Android (`Network.java` : Wi-Fi d'abord, avec son nom ; sinon une interface filaire), toutes les 2 s. Le jeu va directement de « Multijoueur » à « Connecter l'écran » : sa liste Wi-Fi n'apparaît jamais (le pont la quitte aussitôt), ni « Créer le réseau Explorer3 ».

`jni/mirror_android.cpp` est le serveur de `diffusion.cpp` en sockets POSIX, sur un fil :

- `http://<adresse>:8080/` : la page (`page.inc`) et `/sym.js` ;
- `ws://<adresse>:8081/` : poignée de main WebSocket, lignes de l'écran compressées (25 images/s au plus, seulement les lignes changées, tout l'écran à chaque nouvelle connexion, sauf le bandeau), messages `P` (horloge, 250 ms), `T` `S` `R` (son), `K` (pages), `L` (langue), `B` (bandeau), `M` (thème du labyrinthe, avant la page et à chaque connexion) ; ping toutes les 5 s, déconnexion après 2 sans réponse ; 5 navigateurs au plus.

L'écran d'adresse affiche `adresse:8080` (texte et QR code). On ne peut pas continuer tant qu'aucun centre de contrôle n'est connecté.

## 10. Multijoueur : le Centre de contrôle

`libexplorer3_control.so` = le cœur commun avec Explorer3-RaspberryPi (protocole WebSocket, session, horloge, lignes d'image, pages, bandeau, thème du labyrinthe du message `M`, son, saisie d'adresse) + la partie Android (`control.c`, `gl_draw.c`, `net.c`, `ui.c`) :

- **Recherche** (`net.c`, sur un fil) : les 254 adresses du /24 de l'appareil, ports **81** (Cardputer) et **8081** (Équipage Android), par paquets de 96 connexions de 1,5 s ; chaque port ouvert est confirmé par `GET /sym.js` sur 80 ou 8080, avec `Host` = l'adresse (le Cardputer renvoie tout autre nom vers sa page sur son réseau Explorer3). Un équipage trouvé : connexion directe ; plusieurs : liste ; aucun : chercher encore ou taper l'adresse (△).
- **Saisie de l'adresse** : à la croix (L / R ±10), ou au doigt sur le clavier chiffres d'Android, déjà remplie ; vérifiée sur les deux ports avant la connexion.
- **Tactile** : les écrans notent ce qui se touche en se dessinant (`ui_tap()`) : les indications des boutons, les équipages de la liste, l'adresse. Sans manette au démarrage : boutons transparents retour et quitter, mots encadrés à la place des boutons dans les indications.
- **Flux** : comme dans un navigateur. Copie de l'écran, pages, bandeau, son ; reconnexion toute seule après une coupure ou 30 s de silence ; ○ revient à la recherche (rappelé 5 s au début).
- Textes : ceux de la page du jeu (`page_texts.inc`, section 3) ; écrans propres à Android dans `ui.c`, sans le mot « Cardputer ». Les boutons y sont dessinés comme ceux de l'appareil.
- Réglages : `control.conf`, lignes `ip=` (dernier équipage qui a répondu) et `lang=fr|en` (dernière langue de l'équipage).

## 11. Affichage net

**Jeu** (`Screen.java`, OpenGL ES 2) : l'image 240 × 135 est agrandie d'un nombre entier de fois k **sans lissage** dans une texture intermédiaire, puis amenée à la taille finale (le plus grand 16:9 qui tient, centré) **avec lissage** : seuls les bords des gros pixels s'adoucissent. Sur une OUYA en 1080p, k = 8 et la taille finale est exacte. Sur le Xperia Play (854 × 480), k = 3 puis 720 × 405 → 853 × 480.

**Centre de contrôle** (`gl_draw.c`) : la toile 960 × 540 de `draw.h` est mise à l'échelle de l'écran. Rectangles et textes sont tracés à la résolution de l'écran (police 8 × 16 agrandie 4 fois puis lissée) ; la copie de l'écran de l'équipage suit la même méthode que le jeu.

**Page web** (sur l'ordinateur) : celle du Cardputer, qui remplit déjà la fenêtre en gardant les pixels nets (`image-rendering: pixelated`).

## 12. Essais

**Xperia Play** (utilisateur, 2026-10-10) : jeu, son, libellés, clavier ; multijoueur en Équipage avec un navigateur ; Centre de contrôle face à un Cardputer v2.2.2 (curseur du picross compris) ; Start + Select ; lanceur PlayStation Pocket.

**Émulateur** (AVD Android 5.1 x86, `tools/build.sh --x86`, touches par `adb shell input keyevent`, captures par `adb exec-out screencap`) : parcours du Solo ; claviers à l'écran ; choix du rôle ; serveur de l'Équipage contrôlé par un client WebSocket Python et par Edge sans fenêtre ; Centre de contrôle face à un faux Cardputer en Python (avec un petit serveur `/sym.js` sur le port 80). Dans l'émulateur, les adresses 10.0.2.1 à .7 mènent toutes au PC : la recherche y trouve plusieurs fois le même faux Cardputer.

**Version 3.0.0** (utilisateur, 2026-10-13) : validée sur l'appareil (tactile et nouveau Centre de contrôle).

**Pas encore essayé** : l'OUYA ; un Centre de contrôle Android face à un Équipage Android.

## 13. La suite

- Essais sur l'OUYA (prévus) et de deux appareils Android ensemble.
- Toute nouvelle version du jeu : vérifier que `tools/build.sh` passe (il s'arrête si une modification ne s'applique plus), puis rejouer les essais.
