package io.github.ryosaeba89.explorer3;

import android.opengl.GLSurfaceView;
import android.os.Handler;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Mission control's frames: everything is drawn natively (jni/control/gl_draw.c). */
final class ControlRenderer implements GLSurfaceView.Renderer {
    private final Handler activity;
    private int width, height;

    ControlRenderer(Handler activity) {
        this.activity = activity;
    }

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        Control.surfaceCreated();
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int w, int h) {
        width = w;
        height = h;
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        if (!Control.frame(width, height)) {
            Control.requestQuit(activity);
        }
    }
}
