package io.github.ryosaeba89.explorer3;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import java.util.Arrays;

/**
 * Mission control's sound: the web page's synthesis (the notes, the take-off
 * rumble) rendered natively at 48 kHz stereo, written to an AudioTrack. One
 * thread per process; silenced while the app is in the background.
 */
final class ControlAudio implements Runnable {
    static final int RATE = 48000;   // SOUND_RATE of jni/control/sound.h
    private static final int FRAMES = 1024;

    private static ControlAudio instance;
    private volatile boolean muted;

    private ControlAudio() {}

    static synchronized ControlAudio start() {
        if (instance == null) {
            instance = new ControlAudio();
            Thread t = new Thread(instance, "control sound");
            t.setPriority(Thread.MAX_PRIORITY);
            t.start();
        }
        return instance;
    }

    void setMuted(boolean m) {
        muted = m;
    }

    @Override
    public void run() {
        int min = AudioTrack.getMinBufferSize(RATE, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        int bytes = Math.max(min, FRAMES * 4 * 3);
        AudioTrack track = new AudioTrack(AudioManager.STREAM_MUSIC, RATE, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, bytes, AudioTrack.MODE_STREAM);
        track.play();
        int latencyMs = bytes / 4 * 1000 / RATE;  // a sample written now plays once the buffer ahead of it has
        short[] block = new short[FRAMES * 2];
        for (;;) {
            Control.mix(block, latencyMs);
            if (muted) {
                Arrays.fill(block, (short) 0);
            }
            track.write(block, 0, block.length);  // waits while the buffer is full
        }
    }
}
