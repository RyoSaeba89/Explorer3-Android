package io.github.ryosaeba89.explorer3;

import android.os.Build;
import android.view.InputDevice;

/**
 * Which device the app runs on, and whether it has a gamepad, decided once
 * when the app starts (user's choice):
 * <pre>
 *   OUYA                          its buttons in the texts (O U Y A)
 *   Xperia Play                   its buttons in the texts (X O triangle square)
 *   other, gamepad plugged in     the Xperia Play's texts (PlayStation symbols)
 *   other, no gamepad             texts for the finger, transparent buttons
 * </pre>
 * Touching the screen works on all of them.
 */
final class Device {
    private Device() {}

    private static final int SOURCE_GAMEPAD = 0x00000401;   // API 12
    private static final int SOURCE_JOYSTICK = 0x01000010;  // API 12

    static final boolean OUYA = Build.MANUFACTURER.toUpperCase().startsWith("OUYA")
            || Build.MODEL.toUpperCase().startsWith("OUYA");

    /** Sony Ericsson Xperia Play (R800i, R800a, R800at, R800x, SO-01D: "zeus"). */
    static final boolean XPERIA_PLAY = Build.DEVICE.toLowerCase().startsWith("zeus")
            || Build.MODEL.toUpperCase().startsWith("R800")
            || Build.MODEL.toUpperCase().startsWith("SO-01D");

    /** No gamepad found at start: texts for the finger and transparent buttons. */
    static final boolean TOUCH = !OUYA && !XPERIA_PLAY && !gamepadPlugged();

    /** The buttons the texts name, as jni/control/ui.h: 0 PlayStation, 1 OUYA, 2 touch. */
    static final int BUTTONS = OUYA ? 1 : TOUCH ? 2 : 0;

    private static boolean gamepadPlugged() {
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            int s = d == null ? 0 : d.getSources();
            if ((s & SOURCE_GAMEPAD) == SOURCE_GAMEPAD || (s & SOURCE_JOYSTICK) == SOURCE_JOYSTICK) {
                return true;
            }
        }
        return false;
    }
}
