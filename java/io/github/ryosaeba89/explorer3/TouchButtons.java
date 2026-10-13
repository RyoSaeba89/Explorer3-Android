package io.github.ryosaeba89.explorer3;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Typeface;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * Transparent buttons for what has no place on the screen: back, quit,
 * pause / resume, Morse code, signal again. Only when no gamepad was found at
 * start (Device.TOUCH), and only those useful on the current screen (user's
 * choice). They sit in the black bands beside the picture when the screen is
 * wider than it, otherwise over the picture's corners, more transparent:
 * <pre>
 *   back (←)        pause (‖) / resume (▶)
 *                   Morse code (?)
 *   quit (×)        signal again (▶)
 * </pre>
 * The texts for the finger (tools/touch-texts.sed) name them by these signs.
 * Layout and drawing on the GL thread, hit tests on the UI thread.
 */
final class TouchButtons {
    private static final String TAG = "Explorer3";

    // Same bits as input::TouchButton (jni/input.h)
    static final int BACK = 1, QUIT = 2, PAUSE = 4, HELP = 8, REPLAY = 16;
    static final int PAUSED = 32;  // not a button: the pause button shows "resume"

    private static final int[] BITS = {BACK, QUIT, PAUSE, HELP, REPLAY};
    // Pictures in the texture, one square cell each
    private static final int ICON_BACK = 0, ICON_QUIT = 1, ICON_PAUSE = 2, ICON_HELP = 3, ICON_PLAY = 4;
    private static final int ICONS = 5;

    private static final float MIN_DP = 44;   // smallest button: fits a finger
    private static final float MAX_DP = 80;
    private static final float ALPHA_BAND = 0.55f, ALPHA_OVER = 0.35f, ALPHA_PRESSED = 0.9f;

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
            + "uniform float uAlpha;\n"
            + "varying vec2 vTex;\n"
            + "void main() {\n"
            + "  gl_FragColor = texture2D(uTex, vTex) * uAlpha;\n"
            + "}\n";

    private final float density;
    // Centre (x, y from the top) of each button of BITS, and their radius, in pixels
    private final float[] cx = new float[BITS.length];
    private final float[] cy = new float[BITS.length];
    private float radius;
    private boolean inBands;
    private int surfaceW, surfaceH;
    private volatile int shown;    // bits drawn last
    private volatile int pressed;  // bit under the finger, or 0

    private int program, aPos, aTex, uAlpha;
    private int texture, iconSize, textureW, textureH;
    private final FloatBuffer positions = ByteBuffer.allocateDirect(8 * 4)
            .order(ByteOrder.nativeOrder()).asFloatBuffer();
    private final FloatBuffer coords = ByteBuffer.allocateDirect(8 * 4)
            .order(ByteOrder.nativeOrder()).asFloatBuffer();

    TouchButtons(float density) {
        this.density = density;
    }

    /** GL thread: a new context. */
    void surfaceCreated() {
        program = Screen.program(VERTEX_SHADER, FRAGMENT_SHADER);
        aPos = GLES20.glGetAttribLocation(program, "aPos");
        aTex = GLES20.glGetAttribLocation(program, "aTex");
        uAlpha = GLES20.glGetUniformLocation(program, "uAlpha");
        texture = 0;
    }

    /**
     * GL thread: the surface is w x h and the picture px, py (from the top),
     * pw x ph. Side bands wide enough: the buttons go there; otherwise over
     * the picture's corners.
     */
    synchronized void layout(int w, int h, int px, int py, int pw, int ph) {
        surfaceW = w;
        surfaceH = h;
        float band = Math.min(px, w - px - pw);
        float min = MIN_DP * density;
        float max = MAX_DP * density;
        inBands = band >= min;
        float size;
        float left, right, top, middle, bottom;
        if (inBands) {
            size = Math.min(Math.min(band * 0.8f, h / 5f), max);
            float margin = size * 0.4f;
            left = px / 2f;
            right = px + pw + (w - px - pw) / 2f;
            top = margin + size / 2;
            middle = h / 2f;
            bottom = h - margin - size / 2;
        } else {
            size = Math.max(min, Math.min(ph / 7f, max * 0.9f));
            float margin = size * 0.25f;
            left = px + margin + size / 2;
            right = px + pw - margin - size / 2;
            top = py + margin + size / 2;
            middle = py + ph / 2f;
            bottom = py + ph - margin - size / 2;
        }
        radius = size / 2;
        place(BACK, left, top);
        place(QUIT, left, bottom);
        place(PAUSE, right, top);
        place(HELP, right, middle);
        place(REPLAY, right, bottom);
        makeIcons(Math.max(16, Math.round(size)));
        Log.i(TAG, "touch buttons " + Math.round(size) + " px, " + (inBands ? "in the bands" : "over the picture"));
    }

    private void place(int bit, float x, float y) {
        int i = index(bit);
        cx[i] = x;
        cy[i] = y;
    }

    private static int index(int bit) {
        for (int i = 0; i < BITS.length; i++) {
            if (BITS[i] == bit) {
                return i;
            }
        }
        return 0;
    }

    /** UI thread: the button under (x, y) among the ones shown, or 0. */
    synchronized int at(float x, float y) {
        int bits = shown;
        float reach = radius * 1.2f;  // a little more than drawn: fingers are wide
        for (int i = 0; i < BITS.length; i++) {
            float dx = x - cx[i];
            float dy = y - cy[i];
            if ((bits & BITS[i]) != 0 && dx * dx + dy * dy <= reach * reach) {
                return BITS[i];
            }
        }
        return 0;
    }

    /** UI thread: the button the finger is on (drawn brighter), or 0. */
    void press(int bit) {
        pressed = bit;
    }

    /** GL thread: draws the buttons of bits over what is drawn already. */
    synchronized void draw(int bits) {
        shown = bits & (BACK | QUIT | PAUSE | HELP | REPLAY);
        if (shown == 0 || texture == 0) {
            return;
        }
        GLES20.glViewport(0, 0, surfaceW, surfaceH);
        GLES20.glUseProgram(program);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_ONE, GLES20.GL_ONE_MINUS_SRC_ALPHA);  // premultiplied pictures
        GLES20.glEnableVertexAttribArray(aPos);
        GLES20.glEnableVertexAttribArray(aTex);
        for (int i = 0; i < BITS.length; i++) {
            int bit = BITS[i];
            if ((shown & bit) == 0) {
                continue;
            }
            int icon = bit == BACK ? ICON_BACK : bit == QUIT ? ICON_QUIT : bit == HELP ? ICON_HELP
                    : bit == REPLAY || (bits & PAUSED) != 0 ? ICON_PLAY : ICON_PAUSE;
            float alpha = pressed == bit ? ALPHA_PRESSED : inBands ? ALPHA_BAND : ALPHA_OVER;
            quad(cx[i], cy[i], icon);
            GLES20.glUniform1f(uAlpha, alpha);
            GLES20.glVertexAttribPointer(aPos, 2, GLES20.GL_FLOAT, false, 0, positions);
            GLES20.glVertexAttribPointer(aTex, 2, GLES20.GL_FLOAT, false, 0, coords);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        }
        GLES20.glDisableVertexAttribArray(aPos);
        GLES20.glDisableVertexAttribArray(aTex);
        GLES20.glDisable(GLES20.GL_BLEND);
    }

    // A square of the icon's size centred on (x, y), in OpenGL's coordinates
    private void quad(float x, float y, int icon) {
        float half = iconSize / 2f;
        float x0 = (x - half) / surfaceW * 2 - 1;
        float x1 = (x + half) / surfaceW * 2 - 1;
        float yTop = 1 - (y - half) / surfaceH * 2;
        float yBottom = 1 - (y + half) / surfaceH * 2;
        strip(positions, x0, x1, yBottom, yTop);
        float u0 = icon * iconSize / (float) textureW;
        float u1 = (icon + 1) * iconSize / (float) textureW;
        float v1 = iconSize / (float) textureH;  // the bitmap's first row is t = 0
        strip(coords, u0, u1, v1, 0);
    }

    // Two triangles: bottom left, bottom right, top left, top right
    private static void strip(FloatBuffer b, float left, float right, float bottom, float top) {
        b.put(0, left).put(1, bottom).put(2, right).put(3, bottom);
        b.put(4, left).put(5, top).put(6, right).put(7, top);
        b.position(0);
    }

    // The pictures, drawn at the buttons' size so that they stay sharp
    private void makeIcons(int size) {
        iconSize = size;
        textureW = powerOfTwo(size * ICONS);
        textureH = powerOfTwo(size);
        Bitmap b = Bitmap.createBitmap(textureW, textureH, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(b);
        float s = size / 128f;  // pictures designed on 128 x 128
        Paint disc = new Paint(Paint.ANTI_ALIAS_FLAG);
        disc.setColor(0x99000000);
        Paint ring = new Paint(Paint.ANTI_ALIAS_FLAG);
        ring.setColor(0xffffffff);
        ring.setStyle(Paint.Style.STROKE);
        ring.setStrokeWidth(6 * s);
        Paint line = new Paint(ring);
        line.setStrokeWidth(10 * s);
        line.setStrokeCap(Paint.Cap.ROUND);
        line.setStrokeJoin(Paint.Join.ROUND);
        Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
        fill.setColor(0xffffffff);
        for (int i = 0; i < ICONS; i++) {
            c.save();
            c.translate(i * size, 0);
            c.scale(s, s);
            c.drawCircle(64, 64, 60, disc);
            c.drawCircle(64, 64, 57, ring);
            switch (i) {
                case ICON_BACK:
                    c.drawLine(38, 64, 90, 64, line);
                    c.drawLine(38, 64, 60, 42, line);
                    c.drawLine(38, 64, 60, 86, line);
                    break;
                case ICON_QUIT:
                    c.drawLine(44, 44, 84, 84, line);
                    c.drawLine(84, 44, 44, 84, line);
                    break;
                case ICON_PAUSE:
                    c.drawRect(44, 38, 58, 90, fill);
                    c.drawRect(70, 38, 84, 90, fill);
                    break;
                case ICON_HELP:
                    fill.setTextAlign(Paint.Align.CENTER);
                    fill.setTypeface(Typeface.DEFAULT_BOLD);
                    fill.setTextSize(80);
                    c.drawText("?", 64, 92, fill);
                    break;
                default: {  // ICON_PLAY
                    Path p = new Path();
                    p.moveTo(50, 38);
                    p.lineTo(94, 64);
                    p.lineTo(50, 90);
                    p.close();
                    c.drawPath(p, fill);
                    break;
                }
            }
            c.restore();
        }
        if (texture == 0) {
            int[] id = new int[1];
            GLES20.glGenTextures(1, id, 0);
            texture = id[0];
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, b, 0);
        b.recycle();
    }

    private static int powerOfTwo(int n) {
        int p = 1;
        while (p < n) {
            p <<= 1;
        }
        return p;
    }
}
