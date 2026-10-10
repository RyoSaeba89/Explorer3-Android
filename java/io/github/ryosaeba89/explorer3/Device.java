package io.github.ryosaeba89.explorer3;

import android.os.Build;

/** Which device the app runs on: the OUYA, or the Xperia Play (any other device). */
final class Device {
    private Device() {}

    static final boolean OUYA = Build.MANUFACTURER.toUpperCase().startsWith("OUYA")
            || Build.MODEL.toUpperCase().startsWith("OUYA");
}
