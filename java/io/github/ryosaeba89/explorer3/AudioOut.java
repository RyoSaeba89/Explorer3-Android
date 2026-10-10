package io.github.ryosaeba89.explorer3;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * Plays the game's sound: asks the native speaker (jni/audio.cpp) for blocks
 * of 22 050 Hz mono samples and writes them to an AudioTrack. One thread per
 * process, like the game thread. While the app is in the background the game
 * keeps running (user's choice) but its sound is silenced.
 */
final class AudioOut implements Runnable {
    static final int RATE = 22050;  // same as jni/audio.cpp
    private static final int BLOCK = 512;

    private static AudioOut instance;
    private volatile boolean muted;

    private AudioOut() {}

    /** Starts the sound once per process. */
    static synchronized AudioOut start() {
        if (instance == null) {
            instance = new AudioOut();
            Thread t = new Thread(instance, "sound");
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
        int min = AudioTrack.getMinBufferSize(RATE, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        AudioTrack track = new AudioTrack(AudioManager.STREAM_MUSIC, RATE, AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT, Math.max(min, BLOCK * 2 * 4), AudioTrack.MODE_STREAM);
        track.play();
        short[] block = new short[BLOCK];
        for (;;) {
            Game.mix(block);  // the game's sounds advance even when muted
            if (muted) {
                java.util.Arrays.fill(block, (short) 0);
            }
            track.write(block, 0, BLOCK);  // waits while the buffer is full
        }
    }
}
