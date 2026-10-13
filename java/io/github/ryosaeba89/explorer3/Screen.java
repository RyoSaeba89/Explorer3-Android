package io.github.ryosaeba89.explorer3;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Shows the game's 240 x 135 screen as large as possible without distorting
 * it, and sharp: the picture is first enlarged a whole number of times
 * without smoothing (all pixels identical), then brought to its final size
 * with a smoothing that only softens the edges of the big pixels. At an exact
 * multiple (OUYA in 1080p: x 8) the second step changes nothing.
 */
final class Screen implements GLSurfaceView.Renderer {
    private static final String TAG = "Explorer3";

    private static final String VERTEX_SHADER =
            "attribute vec2 aPos;\n"
            + "attribute vec2 aTex;\n"
            + "varying vec2 vTex;\n"
            + "void main() {\n"
            + "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
            + "  vTex = aTex;\n"
            + "}\n";
    private static final String FRAGMENT_SHADER =
            "precision mediump float;\n"
            + "uniform sampler2D uTex;\n"
            + "varying vec2 vTex;\n"
            + "void main() {\n"
            + "  gl_FragColor = texture2D(uTex, vTex);\n"
            + "}\n";

    // A full-viewport rectangle (strip of two triangles)
    private static final float[] POSITIONS = {-1, -1, 1, -1, -1, 1, 1, 1};
    // Game picture: its first row (t = 0) at the top
    private static final float[] GAME_COORDS = {0, 1, 1, 1, 0, 0, 1, 0};
    // Enlarged picture: drawn by OpenGL, so its first row is at the bottom
    private static final float[] LARGE_COORDS = {0, 0, 1, 0, 0, 1, 1, 1};

    static final int MSG_OPEN_CONTROL = 1;

    private final Handler activity;
    private final TouchButtons buttons;  // null with a gamepad
    private final ByteBuffer pixels =
            ByteBuffer.allocateDirect(Game.W * Game.H * 2).order(ByteOrder.nativeOrder());
    private final FloatBuffer positions = floats(POSITIONS);
    private final FloatBuffer gameCoords = floats(GAME_COORDS);
    private final FloatBuffer largeCoords = floats(LARGE_COORDS);

    private int program, aPos, aTex;
    private int gameTexture;      // 240 x 135, as the game draws it
    private int largeTexture;     // enlarged k times, without smoothing
    private int framebuffer;      // to draw into largeTexture
    private int k = 1;
    private boolean twoSteps;     // false if the driver refuses the enlarged texture
    private int vx, vy, vw, vh;   // where the picture goes on the screen
    private int top;              // its top, from the top of the screen (vy is from the bottom)
    private int seen = -1;        // number of the last picture received
    private boolean enlarge;      // new picture to enlarge

    Screen(Handler activity, TouchButtons buttons) {
        this.activity = activity;
        this.buttons = buttons;
    }

    /**
     * UI thread: the point of the game's picture (W x H) under (x, y) of the
     * screen, or null outside the picture.
     */
    synchronized int[] toGame(float x, float y) {
        if (vw == 0 || x < vx || x >= vx + vw || y < top || y >= top + vh) {
            return null;
        }
        return new int[] {(int) ((x - vx) * Game.W / vw), (int) ((y - top) * Game.H / vh)};
    }

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        if (buttons != null) {
            buttons.surfaceCreated();
        }
        program = program(VERTEX_SHADER, FRAGMENT_SHADER);
        aPos = GLES20.glGetAttribLocation(program, "aPos");
        aTex = GLES20.glGetAttribLocation(program, "aTex");
        gameTexture = texture(GLES20.GL_NEAREST);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGB, Game.W, Game.H, 0,
                GLES20.GL_RGB, GLES20.GL_UNSIGNED_SHORT_5_6_5, null);
        largeTexture = 0;
        framebuffer = 0;
        seen = -1;  // new context: upload the picture again
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int w, int h) {
        float scale = Math.min(w / (float) Game.W, h / (float) Game.H);
        synchronized (this) {
            vw = Math.round(Game.W * scale);
            vh = Math.round(Game.H * scale);
            vx = (w - vw) / 2;
            vy = (h - vh) / 2;
            top = h - vy - vh;
        }
        if (buttons != null) {
            buttons.layout(w, h, vx, top, vw, vh);
        }

        int[] max = new int[1];
        GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, max, 0);
        k = Math.max(1, (int) scale);
        while (k > 1 && Game.W * k > max[0]) {
            k--;
        }
        if (largeTexture != 0) {
            GLES20.glDeleteTextures(1, new int[] {largeTexture}, 0);
            GLES20.glDeleteFramebuffers(1, new int[] {framebuffer}, 0);
        }
        largeTexture = texture(GLES20.GL_LINEAR);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, Game.W * k, Game.H * k, 0,
                GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        int[] id = new int[1];
        GLES20.glGenFramebuffers(1, id, 0);
        framebuffer = id[0];
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
                GLES20.GL_TEXTURE_2D, largeTexture, 0);
        twoSteps = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER)
                == GLES20.GL_FRAMEBUFFER_COMPLETE;
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        Log.i(TAG, "screen " + w + "x" + h + ", picture " + vw + "x" + vh + ", x" + k
                + (twoSteps ? "" : " (no intermediate enlargement)"));
        enlarge = true;
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        if (Game.controlRequested()) {
            activity.sendEmptyMessage(MSG_OPEN_CONTROL);  // mission control chosen
        }
        int n = Game.picture(pixels, seen);
        if (n != seen) {
            seen = n;
            pixels.position(0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, gameTexture);
            GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, Game.W, Game.H,
                    GLES20.GL_RGB, GLES20.GL_UNSIGNED_SHORT_5_6_5, pixels);
            enlarge = true;
        }
        GLES20.glUseProgram(program);
        if (twoSteps && enlarge) {
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);
            GLES20.glViewport(0, 0, Game.W * k, Game.H * k);
            draw(gameTexture, gameCoords);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        }
        enlarge = false;
        GLES20.glClearColor(0, 0, 0, 1);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glViewport(vx, vy, vw, vh);
        if (twoSteps) {
            draw(largeTexture, largeCoords);
        } else {
            draw(gameTexture, gameCoords);
        }
        if (buttons != null) {
            buttons.draw(Game.touchButtons());
        }
    }

    private void draw(int texture, FloatBuffer coords) {
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glVertexAttribPointer(aPos, 2, GLES20.GL_FLOAT, false, 0, positions);
        GLES20.glEnableVertexAttribArray(aPos);
        GLES20.glVertexAttribPointer(aTex, 2, GLES20.GL_FLOAT, false, 0, coords);
        GLES20.glEnableVertexAttribArray(aTex);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }

    private static int texture(int filter) {
        int[] id = new int[1];
        GLES20.glGenTextures(1, id, 0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, id[0]);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, filter);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, filter);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        return id[0];
    }

    static int program(String vertex, String fragment) {
        int p = GLES20.glCreateProgram();
        GLES20.glAttachShader(p, shader(GLES20.GL_VERTEX_SHADER, vertex));
        GLES20.glAttachShader(p, shader(GLES20.GL_FRAGMENT_SHADER, fragment));
        GLES20.glLinkProgram(p);
        int[] ok = new int[1];
        GLES20.glGetProgramiv(p, GLES20.GL_LINK_STATUS, ok, 0);
        if (ok[0] == 0) {
            Log.e(TAG, "program: " + GLES20.glGetProgramInfoLog(p));
        }
        return p;
    }

    private static int shader(int type, String source) {
        int s = GLES20.glCreateShader(type);
        GLES20.glShaderSource(s, source);
        GLES20.glCompileShader(s);
        int[] ok = new int[1];
        GLES20.glGetShaderiv(s, GLES20.GL_COMPILE_STATUS, ok, 0);
        if (ok[0] == 0) {
            Log.e(TAG, "shader: " + GLES20.glGetShaderInfoLog(s));
        }
        return s;
    }

    private static FloatBuffer floats(float[] v) {
        FloatBuffer b = ByteBuffer.allocateDirect(v.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        b.put(v).position(0);
        return b;
    }
}
