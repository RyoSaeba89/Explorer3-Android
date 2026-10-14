package io.github.ryosaeba89.explorer3;

import android.opengl.GLSurfaceView;
import android.os.Handler;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Mission control's frames: everything is drawn natively (jni/control/gl_draw.c),
 * then the transparent back and quit buttons while the finger is used.
 */
final class ControlRenderer implements GLSurfaceView.Renderer {
    private static final int CANVAS_W = 960, CANVAS_H = 540;  // jni/control/draw.h

    private final Handler activity;
    private final TouchButtons buttons;  // shown while the finger is used
    private int width, height;

    ControlRenderer(Handler activity, TouchButtons buttons) {
        this.activity = activity;
        this.buttons = buttons;
    }

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        Control.surfaceCreated();
        if (buttons != null) {
            buttons.surfaceCreated();
        }
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int w, int h) {
        width = w;
        height = h;
        if (buttons != null) {
            // where gl_begin() puts the canvas
            float s = Math.min(w / (float) CANVAS_W, h / (float) CANVAS_H);
            int vw = (int) (CANVAS_W * s + 0.5f);
            int vh = (int) (CANVAS_H * s + 0.5f);
            int vx = (w - vw) / 2;
            int vy = (h - vh) / 2;
            buttons.layout(w, h, vx, h - vy - vh, vw, vh);
        }
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        if (!Control.frame(width, height)) {
            Control.requestQuit(activity);
        }
        if (Control.addressRequested()) {
            activity.sendEmptyMessage(Control.MSG_ADDRESS);
        }
        buttons.draw(Device.style() == Device.TOUCH ? TouchButtons.BACK | TouchButtons.QUIT : 0);
    }
}
