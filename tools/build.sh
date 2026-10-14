#!/bin/sh
# Builds build/explorer3.apk with the SDK tools directly, without Gradle.
#   tools/build.sh          ARM only (Xperia Play, OUYA)
#   tools/build.sh --x86    adds x86, for the PC emulator
# Variables: ANDROID_HOME (SDK), NDK_R15C, NDK_OUT (keep it short: Windows
# limits path length), GAME_DIR (the Cardputer game),
# KEYSTORE and KEYSTORE_PASS (signing key, and a file holding its password;
# the SDK debug key by default; releases use the release key, kept outside
# the repository).
set -e
unset MSYS_NO_PATHCONV  # Git Bash must convert "//c" for cmd.exe

HERE=$(cd "$(dirname "$0")/.." && pwd)
SDK=${ANDROID_HOME:-$LOCALAPPDATA/Android/Sdk}
NDK=${NDK_R15C:-$HOME/Android/android-ndk-r15c}
NDK_OUT=${NDK_OUT:-$HOME/Android/o}
GAME=${GAME_DIR:-$HERE/../Escape-Game-Cardputer}
BT=$SDK/build-tools/34.0.0
JAR=$SDK/platforms/android-16/android.jar
KEYSTORE=${KEYSTORE:-$HOME/.android/debug.keystore}
if [ -n "$KEYSTORE_PASS" ]; then
    PASS="file:$(cygpath -m "$KEYSTORE_PASS" 2>/dev/null || echo "$KEYSTORE_PASS")"
else
    PASS=pass:android  # the SDK debug key
fi
B=$HERE/build
ABIS=armeabi-v7a
if [ "$1" = "--x86" ]; then
    ABIS=armeabi-v7a+x86
fi

win() { cygpath -m "$1" 2>/dev/null || echo "$1"; }

echo "== Android texts"
# One copy of the game sources (main.cpp includes "textes.h" from its own
# folder), with the read-only bridge appended to main.cpp, and the game's
# texts in six versions; tr() gives the one of the device used last
# (padText() in bridge.inc, user's choice E1.1), any of them at any time:
#   style     namespace      confirm back keyboard left help pause
#   keyboard  textes         the Cardputer's key names, Ctrl for Fn
#   xperia    textes_xperia  ×       ○    △        □    L    R
#   ouya      textes_ouya    O       A    Y        U    L1   R1
#   ps        textes_ps      ×       ○    △        □    L1   Options
#   xbox      textes_xbox    A       B    Y        X    LB   Start
#   touch     textes_touch   (texts for the finger, tools/touch-texts.sed)
# A gamepad other than the Xperia Play's and the OUYA's: PlayStation or Xbox
# names depending on its maker (Device.java, user's choice E2.2).
# Touch (bridge.inc) knows where the game draws what it can be tapped on:
# stop if one of those has moved in a newer version of the game.
for drawn in 'drawChoices(names, infos, 2, langSel, 32, 32, 8);' \
             'drawChoices(names, infos, 3, modeSel, 31, 28, 2);' \
             'drawChoices(names, infos, 2, roleSel, 26, 38, 6);' \
             'canvas.fillRoundRect(16, y, W - 32, h, 5,' \
             'drawLamp(186, 74, lampOn(now));' 'drawSpeaker(186, 74, morsePlaying);' \
             'canvas.fillRoundRect(cx - 32, cy - 32, 64, 64, 6, C_PANEL2);' \
             'int x = 4 + (i % 2) * 118;' 'int y = 75 + (i / 2) * 23;' \
             'canvas.fillRoundRect(x, y, 114, 21, 4,' \
             'const int cell = 17;' 'const int gx = 4;' 'const int gy = 34;' \
             'const int gx = 35;' 'const int gy = 47;' \
             'int x = 96 + (k % 3) * 48;' 'int y = 18 + (k / 3) * 34;' \
             'canvas.fillRoundRect(x, y, 46, 32, 3, C_PANEL2);'; do
    grep -qF "$drawn" "$GAME/src/main.cpp" || { echo "main.cpp no longer draws: $drawn (touch, see jni/bridge.inc)"; exit 1; }
done
SRC_GAME=$NDK_OUT/game
dst=$SRC_GAME
mkdir -p "$dst"
sed -f "$HERE/tools/android-game.sed" "$GAME/src/main.cpp" | cat - "$HERE/jni/bridge.inc" > "$dst/main.cpp.new"
grep -q 'clientCount() > 0) hint(tr(ADDR_CONTINUE)' "$dst/main.cpp.new" || { echo "tools/android-game.sed no longer applies to main.cpp"; exit 1; }
grep -q 'return padText(t, lang);' "$dst/main.cpp.new" || { echo "tools/android-game.sed no longer applies (tr)"; exit 1; }
grep -q 'return tr(multiMode ? multi : solo);' "$dst/main.cpp.new" || { echo "tools/android-game.sed no longer applies (trm)"; exit 1; }
grep -q '^#include "controle.h"$' "$dst/main.cpp.new" || { echo "the game no longer includes controle.h (mission control role)"; exit 1; }
if grep -q 'controle/\|textes_controle' "$dst/main.cpp.new"; then
    echo "tools/android-game.sed no longer applies (the game's own mission control)"
    exit 1
fi
# The keyboard's texts: the Cardputer's key names (a plugged-in keyboard has
# them), Ctrl pressed 3 times for the game master's pause (Fn)
sed 's/"Fn Fn Fn : reprendre", "Fn Fn Fn: resume"/"Ctrl Ctrl Ctrl : reprendre", "Ctrl Ctrl Ctrl: resume"/' "$GAME/src/textes.h" > "$dst/textes.h.new"
grep -q '"Ctrl Ctrl Ctrl : reprendre"' "$dst/textes.h.new" || { echo "textes.h: the pause text changed (tools/build.sh)"; exit 1; }
version() {  # version <namespace> <sed buttons> [touch texts]
    touch_texts=${3:-/dev/null}  # an empty sed script changes nothing
    sed -f "$HERE/tools/android-texts.sed" "$GAME/src/textes.h" | sed -f "$touch_texts" | sed "$2" |
        sed "s/^namespace textes {/namespace $1 {/; s/^}  \/\/ namespace textes$/}  \/\/ namespace $1/" > "$dst/$1.h.new"
    grep -q "^namespace $1 {" "$dst/$1.h.new" || { echo "textes.h: namespace not found"; exit 1; }
}
version textes_xperia 's/@OK@/×/g; s/@BACK@/○/g; s/@KB@/△/g; s/@SQ@/□/g; s/@L@/L/g; s/@R@/R/g'
version textes_ouya 's/@OK@/O/g; s/@BACK@/A/g; s/@KB@/Y/g; s/@SQ@/U/g; s/@L@/L1/g; s/@R@/R1/g'
version textes_ps 's/@OK@/×/g; s/@BACK@/○/g; s/@KB@/△/g; s/@SQ@/□/g; s/@L@/L1/g; s/@R@/Options/g'
version textes_xbox 's/@OK@/A/g; s/@BACK@/B/g; s/@KB@/Y/g; s/@SQ@/X/g; s/@L@/LB/g; s/@R@/Start/g'
version textes_touch '' "$HERE/tools/touch-texts.sed"
# The texts side by side: every "constexpr Tx NAME" and "constexpr Tx NAME[n]"
# (the order of the versions is that of labels::Style, jni/labels.h)
awk '
/^constexpr Tx [A-Z0-9_]+\[/ {
    name = $3; sub(/\[.*/, "", name)
    printf "for (size_t i = 0; i < sizeof(textes::%s) / sizeof(textes::%s[0]); i++) add(textes::%s[i], textes_xperia::%s[i], textes_ouya::%s[i], textes_ps::%s[i], textes_xbox::%s[i], textes_touch::%s[i]);\n", name, name, name, name, name, name, name, name
    next
}
/^constexpr Tx [A-Z0-9_]+ =/ {
    printf "add(textes::%s, textes_xperia::%s, textes_ouya::%s, textes_ps::%s, textes_xbox::%s, textes_touch::%s);\n", $3, $3, $3, $3, $3, $3
}' "$dst/textes.h.new" > "$dst/texts_table.inc.new"
[ "$(wc -l < "$dst/texts_table.inc.new")" -gt 50 ] || { echo "texts_table.inc: too few texts found in textes.h"; exit 1; }
# Start-up codes and maze pictures (game data, no button names)
cp "$GAME/src/mots.h" "$dst/mots.h.new"
cp "$GAME/src/dessins.h" "$dst/dessins.h.new"
# No explorer3.local on Android (no name announced on the network)
sed 's/HOST_NAME = "explorer3.local"/HOST_NAME = ""/' "$GAME/src/diffusion.h" > "$dst/diffusion.h.new"
awk '/const char PAGE\[\] PROGMEM = R"rawliteral\(/{f=1} f{print} /\)rawliteral";/{f=0}' "$GAME/src/diffusion.cpp" \
    | sed -f "$HERE/tools/android-page.sed" > "$dst/page.inc.new"
if ! grep -q "':8081/'" "$dst/page.inc.new" || grep -E "(cx|[a-z]{2}):'[^']*Cardputer" "$dst/page.inc.new" \
    || ! grep -q 'lance la partie' "$dst/page.inc.new"; then
    echo "tools/android-page.sed no longer applies to the page of diffusion.cpp"
    exit 1
fi
# No displayed text may contain "Cardputer", nor a placeholder left over;
# on a gamepad or for the finger, nor a Cardputer key name (comments may)
quoted() {
    sed 's#//.*##' "$@" | grep '"' || true
}
if quoted "$dst/textes.h.new" "$dst/main.cpp.new" "$dst/mots.h.new" | grep -iE 'cardputer|@[A-Z]+@'; then
    echo "text to adapt for Android: extend tools/android-texts.sed"
    exit 1
fi
if quoted "$dst"/textes_*.h.new | grep -E 'ENTRÉE|ENTER|ESC|ESPACE|SPACE|TAB|DEL|Fn Fn|Ctrl|@[A-Z]+@'; then
    echo "text to adapt for a gamepad or the finger: extend tools/android-texts.sed (and tools/touch-texts.sed)"
    exit 1
fi
for f in main.cpp textes.h textes_xperia.h textes_ouya.h textes_ps.h textes_xbox.h textes_touch.h texts_table.inc \
         mots.h dessins.h diffusion.h page.inc; do  # unchanged files keep their date: no rebuild
    if cmp -s "$dst/$f.new" "$dst/$f"; then rm "$dst/$f.new"; else mv "$dst/$f.new" "$dst/$f"; fi
done

echo "== mission control data"
# Mission control's texts and pictures come from the game itself, so they
# always match it: the texts of its web page (tools/page-texts.awk), rule 4
# not naming any device's button (the crew may be a Cardputer, an Android
# device or a Pi), and its tables MORSE, ASTRO, SYMBOLS (jni/control/gamedata.c).
CTRL=$NDK_OUT/control
mkdir -p "$CTRL"
awk '/const char PAGE\[\] PROGMEM = R"rawliteral\(/{f=1} f{print} /\)rawliteral";/{f=0}' "$GAME/src/diffusion.cpp" \
    | sed -f "$HERE/tools/android-page.sed" > "$CTRL/page.inc.tmp"
awk -f "$HERE/tools/page-texts.awk" "$CTRL/page.inc.tmp" > "$CTRL/page_texts.inc.raw" || exit 1
sed 's/appuie sur @OK@\./lance la partie./; s/presses @OK@\./starts the game./' "$CTRL/page_texts.inc.raw" > "$CTRL/page_texts.inc.new"
rm "$CTRL/page.inc.tmp" "$CTRL/page_texts.inc.raw"
if grep -iE 'cardputer|@[A-Z]+@' "$CTRL/page_texts.inc.new" || ! grep -q 'lance la partie' "$CTRL/page_texts.inc.new"; then
    echo "mission control's texts: the page changed (tools/android-page.sed, tools/build.sh)"
    exit 1
fi
awk '/^const char \*const (MORSE\[26\]|ASTRO\[\]|SYMBOLS\[SYM_COUNT\]\[12\]) = \{/{f=1} f{print} f&&/^\};/{f=0}' "$GAME/src/main.cpp" \
    | sed 's/SYMBOLS\[SYM_COUNT\]/SYMBOLS[]/' > "$CTRL/game_tables.inc.new"
if [ "$(grep -c '^const char \*const' "$CTRL/game_tables.inc.new")" != 3 ] \
    || awk '/ASTRO\[\]/{f=1;next} f&&/^\};/{f=0} f' "$CTRL/game_tables.inc.new" | grep -vE '^ *"[^"]{9}",' | grep .; then
    echo "mission control's tables: MORSE, ASTRO (9 x 12) or SYMBOLS changed in main.cpp (jni/control/gamedata.c)"
    exit 1
fi
for f in page_texts.inc game_tables.inc; do
    if cmp -s "$CTRL/$f.new" "$CTRL/$f"; then rm "$CTRL/$f.new"; else mv "$CTRL/$f.new" "$CTRL/$f"; fi
done

echo "== native ($ABIS)"
cd "$HERE"
rm -rf libs
cmd //c "$(cygpath -w "$NDK/ndk-build.cmd") -j8 NDK_MIN_PLATFORM_LEVEL=9 NDK_OUT=$(win "$NDK_OUT") ABIS=$ABIS GAME_DIR=$(win "$GAME") SRC_GAME=$(win "$SRC_GAME") CONTROL_GEN=$(win "$CTRL")"     | grep -v "Compile" || true
for lib in explorer3 explorer3_control; do
    [ -f libs/armeabi-v7a/lib$lib.so ] || { echo "native build failed"; exit 1; }
done

echo "== resources"
rm -rf "$B"
mkdir -p "$B/gen" "$B/classes" "$B/dex" "$B/content"
"$BT/aapt2" compile --dir res -o "$B/res.zip"
"$BT/aapt2" link -o "$B/base.apk" -I "$JAR" --manifest AndroidManifest.xml \
    --min-sdk-version 10 --target-sdk-version 16 --java "$B/gen" "$B/res.zip"

echo "== java"
javac -nowarn -Xlint:-options -source 8 -target 8 -encoding UTF-8 -bootclasspath "$(win "$JAR")" \
    -d "$B/classes" $(find java "$B/gen" -name '*.java')
java -cp "$(win "$BT/lib/d8.jar")" com.android.tools.r8.D8 --release --min-api 10 \
    --lib "$(win "$JAR")" --output "$(win "$B/dex")" $(find "$B/classes" -name '*.class')

echo "== apk"
cp "$B/dex/classes.dex" "$B/content/"
mkdir -p "$B/content/lib"
cp -r libs/* "$B/content/lib/"
jar --update --no-manifest --file "$(win "$B/base.apk")" -C "$(win "$B/content")" .
"$BT/zipalign" -p -f 4 "$B/base.apk" "$B/aligned.apk"
# minSdk 10: apksigner adds the v1 (JAR, SHA-1) signature that Android 2.3 needs
java -jar "$(win "$BT/lib/apksigner.jar")" sign --ks "$(win "$KEYSTORE")" \
    --ks-pass "$PASS" --min-sdk-version 10 \
    --out "$(win "$B/explorer3.apk")" "$(win "$B/aligned.apk")"
java -jar "$(win "$BT/lib/apksigner.jar")" verify --min-sdk-version 10 "$(win "$B/explorer3.apk")"
ls -l "$B/explorer3.apk"
