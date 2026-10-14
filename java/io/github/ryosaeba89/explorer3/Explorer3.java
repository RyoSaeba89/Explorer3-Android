package io.github.ryosaeba89.explorer3;

import android.app.Activity;
import android.content.Intent;
import android.media.AudioManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import java.io.File;

/**
 * The game in full screen, played with the Xperia Play or OUYA gamepad (or a
 * plugged-in keyboard). Buttons by position, the same on both pads:
 * <pre>
 *   bottom  (X  / O)  confirm            right (O  / A)  back
 *   top     (/\ / Y)  on-screen keyboard left  ([] / U)  space (Morse) / erase
 *   L                 help               R               pause
 * </pre>
 * Xperia Play: Start = pause, Select = help, Start + Select = quit.
 * OUYA: Menu = quit.
 * <p>
 * Touch, on every device: the game's screen itself is tapped (jni/bridge.inc);
 * while the finger is used, transparent buttons do the rest (TouchButtons).
 * The texts name the keys of the device used last (Device.java).
 */
public final class Explorer3 extends Activity
        implements Runnable, Handler.Callback, View.OnTouchListener, TouchTarget {
    private static final int KEYCODE_ESCAPE = 111;  // API 11
    private static final int KEYCODE_CTRL_LEFT = 113, KEYCODE_CTRL_RIGHT = 114;  // API 11

    private GLSurfaceView view;
    private Screen screen;
    private TouchInput touch;
    private AudioOut audio;
    private boolean startDown, selectDown;
    private boolean comboUsed;  // Start + Select pressed: neither acts alone
    private boolean controlOpen;  // the Control activity was opened from here
    private final Handler handler = new Handler(this);

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        Game.start(new File(getFilesDir(), "settings.txt").getPath());
        Game.setStyle(Device.style());
        audio = AudioOut.start();
        setVolumeControlStream(AudioManager.STREAM_MUSIC);  // volume buttons set the game's volume
        TouchButtons buttons = new TouchButtons(getResources().getDisplayMetrics().density);
        screen = new Screen(handler, buttons);
        touch = new TouchInput(this, this, buttons);
        view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(screen);
        view.setOnTouchListener(this);
        setContentView(view);
    }

    @Override
    public boolean onTouch(View v, MotionEvent ev) {
        if (Device.touched(ev)) {
            Game.setStyle(Device.TOUCH);  // texts for the finger, transparent buttons
        }
        return touch.onTouch(ev);
    }

    /** A tap on the game's picture; elsewhere (black bands) nothing. */
    @Override
    public void tapped(float x, float y) {
        int[] p = screen.toGame(x, y);
        if (p != null) {
            Game.tap(p[0], p[1]);
        }
    }

    @Override
    public void buttonTapped(int bit) {
        switch (bit) {
            case TouchButtons.BACK: Game.press(Game.BACK); break;
            case TouchButtons.QUIT: finish(); break;
            case TouchButtons.PAUSE: Game.press(Game.PAUSE); break;
            case TouchButtons.HELP: Game.press(Game.HELP); break;
            case TouchButtons.REPLAY: Game.press(Game.SPACE); break;
            default: break;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        view.onPause();  // the game itself keeps running (user's choice)
        handler.removeCallbacks(this);
        audio.setMuted(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        view.onResume();
        handler.post(this);
        audio.setMuted(false);
        if (controlOpen) {
            controlOpen = false;
            Game.controlClosed();  // back from mission control: the game's role menu
        }
    }

    /** Mission control chosen in the game's role menu: opens it; Back there returns here. */
    @Override
    public boolean handleMessage(Message m) {
        if (m.what == Screen.MSG_OPEN_CONTROL) {
            Intent i = new Intent(this, Control.class);
            i.putExtra(Control.EXTRA_LANG, Game.language());
            controlOpen = true;
            startActivity(i);
        }
        return true;
    }

    /** Checks Android's connection every 2 s while the game is in front. */
    @Override
    public void run() {
        updateNetwork();
        handler.postDelayed(this, 2000);
    }

    /** Tells the game which network Android is connected to (Network). */
    private void updateNetwork() {
        String[] net = Network.current(this);
        Game.setNetwork(net[0], net[1]);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isFinishing()) {
            Process.killProcess(Process.myPid());  // stops the game thread: next time it starts afresh
        }
    }

    @Override
    public boolean onKeyDown(int code, KeyEvent ev) {
        if (Device.used(ev)) {
            Game.setStyle(Device.style());  // the texts name this device's keys from now on
        }
        if (code == KeyEvent.KEYCODE_BUTTON_START || code == KeyEvent.KEYCODE_BUTTON_SELECT) {
            if (code == KeyEvent.KEYCODE_BUTTON_START) {
                startDown = true;
            } else {
                selectDown = true;
            }
            if (startDown && selectDown) {
                comboUsed = true;
                finish();
            }
            return true;
        }
        if (Device.OUYA && code == KeyEvent.KEYCODE_MENU) {
            finish();
            return true;
        }
        int game = translate(code, ev);
        if (game == 0) {
            return super.onKeyDown(code, ev);
        }
        if (ev.getRepeatCount() == 0) {  // the Cardputer keyboard does not repeat
            Game.press(game);
        }
        return true;
    }

    @Override
    public boolean onKeyUp(int code, KeyEvent ev) {
        // Start and Select act on release, unless they were pressed together
        if (code == KeyEvent.KEYCODE_BUTTON_START || code == KeyEvent.KEYCODE_BUTTON_SELECT) {
            boolean start = code == KeyEvent.KEYCODE_BUTTON_START;
            if (!comboUsed) {
                Game.press(start ? Game.PAUSE : Game.HELP);
            }
            if (start) {
                startDown = false;
            } else {
                selectDown = false;
            }
            if (!startDown && !selectDown) {
                comboUsed = false;
            }
            return true;
        }
        if (translate(code, ev) != 0) {
            return true;  // otherwise Back (the Xperia Play's circle) would close the game
        }
        return super.onKeyUp(code, ev);
    }

    /** Game code for this button or key, or 0. */
    private static int translate(int code, KeyEvent ev) {
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP: return Game.UP;
            case KeyEvent.KEYCODE_DPAD_DOWN: return Game.DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT: return Game.LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return Game.RIGHT;
            case KeyEvent.KEYCODE_DPAD_CENTER:  // Xperia Play cross
            case KeyEvent.KEYCODE_BUTTON_A:     // OUYA O
                return Game.CONFIRM;
            case KeyEvent.KEYCODE_BACK:         // Xperia Play circle
            case KeyEvent.KEYCODE_BUTTON_B:     // OUYA A
                return Game.BACK;
            case KeyEvent.KEYCODE_BUTTON_Y:     // Xperia Play triangle, OUYA Y
                return Game.KEYBOARD;
            case KeyEvent.KEYCODE_BUTTON_X:     // Xperia Play square, OUYA U
                return Game.SPACE;
            case KeyEvent.KEYCODE_BUTTON_L1: return Game.HELP;
            case KeyEvent.KEYCODE_BUTTON_R1: return Game.PAUSE;
            case KeyEvent.KEYCODE_ENTER: return Game.KEY_ENTER;
            case KEYCODE_ESCAPE: return Game.KEY_ESC;
            case KeyEvent.KEYCODE_DEL: return Game.KEY_DEL;
            case KeyEvent.KEYCODE_TAB: return Game.KEY_TAB;
            case KeyEvent.KEYCODE_SPACE: return Game.KEY_SPACE;
            case KEYCODE_CTRL_LEFT:
            case KEYCODE_CTRL_RIGHT:
                return Game.KEY_CTRL;  // Fn: 3 times for the pause
            default:
                break;
        }
        int c = ev.getUnicodeChar();  // plugged-in keyboard: letters, digits, punctuation
        return c > ' ' && c < 127 ? c : 0;
    }
}
