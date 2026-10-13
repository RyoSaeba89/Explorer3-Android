package io.github.ryosaeba89.explorer3;

import android.content.Context;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

/**
 * Taps on the screen: a finger that goes down and up without moving. On a
 * transparent button, that button; elsewhere, the point where it went down.
 * A second finger cancels. Only the touch screen counts: the Xperia Play's
 * touchpads stay ignored (decision of 2026-10-10). UI thread.
 */
final class TouchInput {
    private final TouchTarget target;
    private final TouchButtons buttons;  // null with a gamepad
    private final float slop;
    private float downX, downY;
    private int downButton;
    private boolean tapping;

    TouchInput(Context context, TouchTarget target, TouchButtons buttons) {
        this.target = target;
        this.buttons = buttons;
        slop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    boolean onTouch(MotionEvent ev) {
        if ((ev.getSource() & InputDevice.SOURCE_TOUCHSCREEN) != InputDevice.SOURCE_TOUCHSCREEN) {
            return false;
        }
        switch (ev.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN:
                downX = ev.getX();
                downY = ev.getY();
                downButton = buttons == null ? 0 : buttons.at(downX, downY);
                press(downButton);
                tapping = true;
                break;
            case MotionEvent.ACTION_MOVE:
                if (Math.abs(ev.getX() - downX) > slop || Math.abs(ev.getY() - downY) > slop) {
                    cancel();
                }
                break;
            case MotionEvent.ACTION_UP:
                if (tapping) {
                    if (downButton != 0) {
                        target.buttonTapped(downButton);
                    } else {
                        target.tapped(downX, downY);
                    }
                }
                cancel();
                break;
            default:  // second finger, cancel
                cancel();
                break;
        }
        return true;
    }

    private void cancel() {
        tapping = false;
        press(0);
    }

    private void press(int bit) {
        if (buttons != null) {
            buttons.press(bit);
        }
    }
}
