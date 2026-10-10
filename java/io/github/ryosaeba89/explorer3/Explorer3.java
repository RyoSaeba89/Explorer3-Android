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
 */
public final class Explorer3 extends Activity implements Runnable, Handler.Callback {
    private static final int KEYCODE_ESCAPE = 111;  // API 11

    private GLSurfaceView view;
    private AudioOut audio;
    private boolean startDown, selectDown;
    private boolean comboUsed;  // Start + Select pressed: neither acts alone
    private final Handler handler = new Handler(this);

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        Game.start(new File(getFilesDir(), "settings.txt").getPath());
        audio = AudioOut.start();
        setVolumeControlStream(AudioManager.STREAM_MUSIC);  // volume buttons set the game's volume
        view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(new Screen(handler));
        setContentView(view);
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
    }

    /** Mission control chosen in the role menu: opens it; Back there returns here. */
    @Override
    public boolean handleMessage(Message m) {
        if (m.what == Screen.MSG_OPEN_CONTROL) {
            Intent i = new Intent(this, Control.class);
            i.putExtra(Control.EXTRA_LANG, Game.language());
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
            default:
                break;
        }
        int c = ev.getUnicodeChar();  // plugged-in keyboard: letters, digits, punctuation
        return c > ' ' && c < 127 ? c : 0;
    }
}
