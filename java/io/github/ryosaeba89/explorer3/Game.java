package io.github.ryosaeba89.explorer3;

import java.nio.ByteBuffer;

/** The Cardputer game compiled natively (jni/game.cpp), on its own thread. */
final class Game {
    static final int W = 240;
    static final int H = 135;

    // Gamepad buttons by role and keyboard keys: same numbers as jni/input.h.
    // From 33 on, a printable character.
    static final int UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4;
    static final int CONFIRM = 5, BACK = 6, KEYBOARD = 7, SPACE = 8, HELP = 9, PAUSE = 10;
    static final int KEY_ENTER = 11, KEY_ESC = 12, KEY_DEL = 13, KEY_TAB = 14, KEY_SPACE = 15;

    static {
        // One build of the game per device: its texts name that device's
        // buttons, or the finger when there is no gamepad
        System.loadLibrary(Device.OUYA ? "explorer3_ouya" : Device.TOUCH ? "explorer3_touch" : "explorer3_xperia");
    }

    private Game() {}

    /** Starts the game, once per process; settings = the file of its memory. */
    static native void start(String settings);

    /** Android's connection: Wi-Fi name (or "Ethernet") and IPv4 address, empty when offline. */
    static native void setNetwork(String name, String ip);

    /** Mixes the game's sound into out (AudioOut.RATE, mono). */
    static native void mix(short[] out);

    /** True once when mission control was chosen in the role menu (after "Multiplayer"). */
    static native boolean controlRequested();

    /** The game's language: 0 French, 1 English. */
    static native int language();

    /** One button or key press (codes above). */
    static native void press(int code);

    /** A tap at (x, y) of the game's picture (W x H). */
    static native void tap(int x, int y);

    /** The transparent buttons useful on the current screen (TouchButtons bits). */
    static native int touchButtons();

    /**
     * Copies the latest picture (RGB565, W x H) into dst, a direct buffer, if
     * its number differs from seen; returns the picture's number.
     */
    static native int picture(ByteBuffer dst, int seen);
}
