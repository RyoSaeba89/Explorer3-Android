package io.github.ryosaeba89.explorer3;

/** What a tap on the screen reaches (TouchInput): an activity. */
interface TouchTarget {
    /** A transparent button (TouchButtons bit). */
    void buttonTapped(int bit);

    /** Anywhere else, in pixels of the screen. */
    void tapped(float x, float y);
}
