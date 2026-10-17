package com.rasyidcode.dotaitemmerge;

/** Monotonic-time confirmation window shared by button and gesture back actions. */
final class BackExitGuard {
    private static final long CONFIRMATION_WINDOW_MS = 2000;
    private boolean armed;
    private long lastPressTime;

    boolean press(long now) {
        if (armed && now >= lastPressTime && now - lastPressTime < CONFIRMATION_WINDOW_MS) {
            reset();
            return true;
        }
        armed = true;
        lastPressTime = now;
        return false;
    }

    void reset() {
        armed = false;
    }
}
