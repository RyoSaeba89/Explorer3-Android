#!/bin/sh
# Builds build/explorer3.apk with the SDK tools directly, without Gradle.
#   tools/build.sh          ARM only (Xperia Play, OUYA)
#   tools/build.sh --x86    adds x86, for the PC emulator
# Variables: ANDROID_HOME (SDK), NDK_R15C, NDK_OUT (keep it short: Windows
# limits path length), GAME_DIR (the Cardputer game), GC_DIR (GC-Explorer3),
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
# One copy of the game sources per device: Android texts with that device's
# buttons, and the read-only bridge appended to main.cpp (main.cpp includes
# "textes.h" from its own folder).
#   device  confirm back keyboard left  help pause
#   xperia  ×       ○    △        □     L    R
#   ouya    O       A    Y        U     L1   R1
copy_game() {  # copy_game <folder> <sed buttons>
    dst=$1
    mkdir -p "$dst"
    sed -f "$HERE/tools/android-game.sed" "$GAME/src/main.cpp" | cat - "$HERE/jni/bridge.inc" > "$dst/main.cpp.new"
    grep -q 'clientCount() > 0) hint(tr(ADDR_CONTINUE)' "$dst/main.cpp.new"         || { echo "tools/android-game.sed no longer applies to main.cpp"; exit 1; }
    sed -f "$HERE/tools/android-texts.sed" "$GAME/src/textes.h" | sed "$2" > "$dst/textes.h.new"
    # No explorer3.local on Android (no name announced on the network)
    sed 's/HOST_NAME = "explorer3.local"/HOST_NAME = ""/' "$GAME/src/diffusion.h" > "$dst/diffusion.h.new"
    awk '/const char PAGE\[\] PROGMEM = R"rawliteral\(/{f=1} f{print} /\)rawliteral";/{f=0}' "$GAME/src/diffusion.cpp"         | sed -f "$HERE/tools/android-page.sed" | sed "$2" > "$dst/page.inc.new"
    if ! grep -q "':8081/'" "$dst/page.inc.new" || grep -E "(cx|[a-z]{2}):'[^']*Cardputer" "$dst/page.inc.new"; then
        echo "tools/android-page.sed no longer applies to the page of diffusion.cpp"
        exit 1
    fi
    # No displayed text may contain "Cardputer" or a Cardputer key name, nor a
    # placeholder left over (comments may)
    texts=$(sed 's#//.*##' "$dst/textes.h.new" "$dst/main.cpp.new" | grep '"' || true)
    if echo "$texts" | grep -i 'cardputer' || echo "$texts" | grep -E 'ENTRÉE|ENTER|ESC|ESPACE|SPACE|TAB|DEL|Fn Fn|@[A-Z]+@'; then
        echo "text to adapt for Android: extend tools/android-texts.sed"
        exit 1
    fi
    for f in main.cpp textes.h diffusion.h page.inc; do  # unchanged files keep their date: no rebuild
        if cmp -s "$dst/$f.new" "$dst/$f"; then rm "$dst/$f.new"; else mv "$dst/$f.new" "$dst/$f"; fi
    done
}
SRC_XPERIA=$NDK_OUT/game-xperia
SRC_OUYA=$NDK_OUT/game-ouya
copy_game "$SRC_XPERIA" 's/@OK@/×/g; s/@BACK@/○/g; s/@KB@/△/g; s/@SQ@/□/g; s/@L@/L/g; s/@R@/R/g'
copy_game "$SRC_OUYA" 's/@OK@/O/g; s/@BACK@/A/g; s/@KB@/Y/g; s/@SQ@/U/g; s/@L@/L1/g; s/@R@/R1/g'

echo "== mission control texts"
# GC-Explorer3's texts for the control role, adapted on a copy (only the
# texts that are displayed on Android are checked)
GC=${GC_DIR:-$HERE/../GC-Explorer3}
CTRL=$NDK_OUT/control
mkdir -p "$CTRL"
sed -f "$HERE/tools/control-texts.sed" "$GC/source/texts.c" > "$CTRL/texts.c.new"
if grep -E '\[TX_(CONNECTING|RULES_4|OLD_GAME)\]' -A1 "$CTRL/texts.c.new" | grep -E 'Cardputer|ENTRÉE|ENTER'; then
    echo "tools/control-texts.sed no longer applies to GC-Explorer3's texts.c"
    exit 1
fi
if cmp -s "$CTRL/texts.c.new" "$CTRL/texts.c"; then rm "$CTRL/texts.c.new"; else mv "$CTRL/texts.c.new" "$CTRL/texts.c"; fi

echo "== native ($ABIS)"
cd "$HERE"
rm -rf libs
cmd //c "$(cygpath -w "$NDK/ndk-build.cmd") -j8 NDK_MIN_PLATFORM_LEVEL=9 NDK_OUT=$(win "$NDK_OUT") ABIS=$ABIS GAME_DIR=$(win "$GAME") SRC_XPERIA=$(win "$SRC_XPERIA") SRC_OUYA=$(win "$SRC_OUYA") GC_DIR=$(win "$GC") CONTROL_TEXTS=$(win "$CTRL")"     | grep -v "Compile" || true
for lib in explorer3_xperia explorer3_ouya explorer3_control; do
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
