package io.github.ryosaeba89.explorer3;

import android.app.Activity;
import android.media.AudioManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;

import java.io.File;
import java.util.Locale;

/**
 * Mission control: this device as the crew's screen, in place of a web
 * browser (jni/control/control.c, with GC-Explorer3's code). Buttons by
 * position, as in the game: D-pad moves, bottom face button (X / O)
 * confirms, right one (circle / A) goes back, top one (triangle / Y) types an
 * address, L / R change it by 10. Quit: Start + Select, or Menu on the OUYA.
 *
 * No inner or anonymous classes: d8 (build-tools 34) fails on the ones
 * compiled by JDK 21 (see MEMOIRE.md).
 */
public final class Control extends Activity implements Runnable, Handler.Callback {
    // Buttons: same numbers as jni/control/control.c
    static final int UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4, OK = 5, BACK = 6, KEYBOARD = 7, L = 8, R = 9;
    private static final int MSG_QUIT = 1;
    static final String EXTRA_LANG = "lang";  // the game's language
    private static final int KEYCODE_ESCAPE = 111;  // API 11

    static {
        System.loadLibrary("explorer3_control");
    }

    /** Once per process: settings file, language before the crew's, OUYA buttons. */
    static native void start(String settings, int lang, boolean ouya);

    static native void setNetwork(String name, String ip);

    static native void button(int code, boolean down);

    static native void surfaceCreated();

    /** One frame on the GL thread; false when the player asked to quit. */
    static native boolean frame(int w, int h);

    /** Stereo samples at ControlAudio.RATE; latencyMs: delay before they are heard. */
    static native void mix(short[] out, int latencyMs);

    private GLSurfaceView view;
    private ControlAudio audio;
    private final Handler handler = new Handler(this);
    private boolean startDown, selectDown;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        int lang = getIntent().getIntExtra(EXTRA_LANG, Locale.getDefault().getLanguage().equals("fr") ? 0 : 1);
        start(new File(getFilesDir(), "control.conf").getPath(), lang, Device.OUYA);
        audio = ControlAudio.start();
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(new ControlRenderer(handler));
        setContentView(view);
    }

    @Override
    protected void onPause() {
        super.onPause();
        view.onPause();
        handler.removeCallbacks(this);
        audio.setMuted(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        view.onResume();
        handler.post(this);
        audio.setMuted(false);
    }

    /** Checks Android's connection every 2 s while in front. */
    @Override
    public void run() {
        String[] net = Network.current(this);
        setNetwork(net[0], net[1]);
        handler.postDelayed(this, 2000);
    }

    /** The GL thread asked to quit. */
    @Override
    public boolean handleMessage(Message m) {
        if (m.what == MSG_QUIT) {
            finish();
        }
        return true;
    }

    static void requestQuit(Handler h) {
        h.sendEmptyMessage(MSG_QUIT);
    }

    @Override
    public boolean onKeyDown(int code, KeyEvent ev) {
        if (code == KeyEvent.KEYCODE_BUTTON_START || code == KeyEvent.KEYCODE_BUTTON_SELECT) {
            if (code == KeyEvent.KEYCODE_BUTTON_START) {
                startDown = true;
            } else {
                selectDown = true;
            }
            if (startDown && selectDown) {
                finish();
            }
            return true;
        }
        if (Device.OUYA && code == KeyEvent.KEYCODE_MENU) {
            finish();
            return true;
        }
        int b = translate(code);
        if (b == 0) {
            return super.onKeyDown(code, ev);
        }
        if (ev.getRepeatCount() == 0) {  // held buttons repeat by themselves (address entry)
            button(b, true);
        }
        return true;
    }

    @Override
    public boolean onKeyUp(int code, KeyEvent ev) {
        if (code == KeyEvent.KEYCODE_BUTTON_START) {
            startDown = false;
            return true;
        }
        if (code == KeyEvent.KEYCODE_BUTTON_SELECT) {
            selectDown = false;
            return true;
        }
        int b = translate(code);
        if (b == 0) {
            return super.onKeyUp(code, ev);
        }
        button(b, false);
        return true;  // otherwise Back (the Xperia Play's circle) would close the app
    }

    private static int translate(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP: return UP;
            case KeyEvent.KEYCODE_DPAD_DOWN: return DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT: return LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return RIGHT;
            case KeyEvent.KEYCODE_DPAD_CENTER:  // Xperia Play cross
            case KeyEvent.KEYCODE_BUTTON_A:     // OUYA O
            case KeyEvent.KEYCODE_ENTER:
                return OK;
            case KeyEvent.KEYCODE_BACK:         // Xperia Play circle
            case KeyEvent.KEYCODE_BUTTON_B:     // OUYA A
            case KEYCODE_ESCAPE:
                return BACK;
            case KeyEvent.KEYCODE_BUTTON_Y:     // Xperia Play triangle, OUYA Y
                return KEYBOARD;
            case KeyEvent.KEYCODE_BUTTON_L1: return L;
            case KeyEvent.KEYCODE_BUTTON_R1: return R;
            default: return 0;
        }
    }
}
