package io.github.ryosaeba89.explorer3;

import android.os.Build;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.lang.reflect.Method;

/**
 * Which device the app runs on, and which device the player used last: the
 * texts, the on-screen keyboard's cursor, the transparent buttons and mission
 * control's hints follow it (user's choice E1.1):
 * <pre>
 *   Xperia Play's own buttons     X O triangle square, L R
 *   OUYA controller               O U Y A, L1 R1
 *   other gamepad                 PlayStation (Sony) or Xbox names (user's choice E2.2)
 *   keyboard                      the keyboard's keys
 *   touch screen                  texts for the finger, transparent buttons
 * </pre>
 * At start: the device's own gamepad (Xperia Play, OUYA), a gamepad plugged
 * in, otherwise the finger.
 */
final class Device {
    private Device() {}

    // The order of labels::Style (jni/labels.h) and of PAD_... (jni/control/ui.h)
    static final int KEYBOARD = 0, XPERIA = 1, OUYA_PAD = 2, PLAYSTATION = 3, XBOX = 4, TOUCH = 5;

    private static final int SOURCE_GAMEPAD = 0x00000401;   // API 12
    private static final int SOURCE_JOYSTICK = 0x01000010;  // API 12
    private static final int VENDOR_SONY = 0x054c;

    static final boolean OUYA = Build.MANUFACTURER.toUpperCase().startsWith("OUYA")
            || Build.MODEL.toUpperCase().startsWith("OUYA");

    /** Sony Ericsson Xperia Play (R800i, R800a, R800at, R800x, SO-01D: "zeus"). */
    static final boolean XPERIA_PLAY = Build.DEVICE.toLowerCase().startsWith("zeus")
            || Build.MODEL.toUpperCase().startsWith("R800")
            || Build.MODEL.toUpperCase().startsWith("SO-01D");

    private static volatile int style = atStart();

    /** The device used last (KEYBOARD ... TOUCH). */
    static int style() {
        return style;
    }

    /** A key or button: true if the style changed. */
    static boolean used(KeyEvent ev) {
        int s = styleOf(ev);
        return s >= 0 && set(s);
    }

    /** A touch: true if the style changed (the touch screen only, not the OUYA's touchpad). */
    static boolean touched(MotionEvent ev) {
        if ((ev.getSource() & InputDevice.SOURCE_TOUCHSCREEN) != InputDevice.SOURCE_TOUCHSCREEN) {
            return false;
        }
        return set(TOUCH);
    }

    private static boolean set(int s) {
        if (s == style) {
            return false;
        }
        style = s;
        return true;
    }

    private static int atStart() {
        if (OUYA) {
            return OUYA_PAD;
        }
        if (XPERIA_PLAY) {
            return XPERIA;
        }
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (d != null && isGamepad(d)) {
                return padStyle(d);
            }
        }
        return TOUCH;
    }

    /**
     * Style of a key, or -1: keep the current one (volume keys, a phone's
     * Back...). Gamepad buttons and the cross: the device's own gamepad, or
     * that gamepad's maker; typing keys of a keyboard: the keyboard.
     */
    private static int styleOf(KeyEvent ev) {
        int code = ev.getKeyCode();
        InputDevice d = ev.getDevice();
        boolean alphabetic = d != null && d.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC;
        boolean padKey = (code >= KeyEvent.KEYCODE_DPAD_UP && code <= KeyEvent.KEYCODE_DPAD_CENTER)
                || (code >= KeyEvent.KEYCODE_BUTTON_A && code <= KeyEvent.KEYCODE_BUTTON_MODE);
        if (!padKey) {
            if (alphabetic) {
                return KEYBOARD;  // letters, Enter, Esc, Tab, Ctrl...
            }
            if (XPERIA_PLAY && code == KeyEvent.KEYCODE_BACK) {
                return XPERIA;  // its circle button
            }
            return -1;
        }
        if (XPERIA_PLAY) {
            return XPERIA;  // its sliding gamepad
        }
        if (OUYA) {
            return OUYA_PAD;  // its controller
        }
        if (d != null && isGamepad(d)) {
            return padStyle(d);
        }
        return alphabetic ? KEYBOARD : -1;  // a keyboard's arrows
    }

    private static boolean isGamepad(InputDevice d) {
        int s = d.getSources();
        return (s & SOURCE_GAMEPAD) == SOURCE_GAMEPAD || (s & SOURCE_JOYSTICK) == SOURCE_JOYSTICK;
    }

    /** A gamepad's names: PlayStation for Sony's, Xbox for the others (8BitDo, Xbox...). */
    private static int padStyle(InputDevice d) {
        String name = d.getName() == null ? "" : d.getName().toLowerCase();
        if (vendor(d) == VENDOR_SONY || name.contains("playstation") || name.contains("dualshock")
                || name.contains("dualsense") || name.equals("wireless controller")) {
            return PLAYSTATION;
        }
        return XBOX;
    }

    /** USB vendor id (Android 4.4 on), or -1. */
    private static int vendor(InputDevice d) {
        if (Build.VERSION.SDK_INT < 19) {
            return -1;
        }
        try {
            Method m = InputDevice.class.getMethod("getVendorId");
            return (Integer) m.invoke(d);
        } catch (Exception e) {
            return -1;
        }
    }
}
