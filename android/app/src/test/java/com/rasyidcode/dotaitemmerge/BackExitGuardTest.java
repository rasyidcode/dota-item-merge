package com.rasyidcode.dotaitemmerge;

import org.junit.Test;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public final class BackExitGuardTest {
    @Test public void firstBackOnlyPromptsEvenAtZeroUptime() {
        assertFalse(new BackExitGuard().press(0));
    }

    @Test public void secondBackWithinTwoSecondsExits() {
        BackExitGuard guard = new BackExitGuard();
        assertFalse(guard.press(100));
        assertTrue(guard.press(2099));
    }

    @Test public void expiredBackStartsANewWindow() {
        BackExitGuard guard = new BackExitGuard();
        assertFalse(guard.press(100));
        assertFalse(guard.press(2100));
        assertTrue(guard.press(2200));
    }

    @Test public void returningFromBackgroundRequiresTwoFreshPresses() {
        BackExitGuard guard = new BackExitGuard();
        assertFalse(guard.press(100));
        guard.reset();
        assertFalse(guard.press(200));
        assertTrue(guard.press(300));
    }

    @Test public void confirmingExitDoesNotLeaveAnArmedWindow() {
        BackExitGuard guard = new BackExitGuard();
        assertFalse(guard.press(100));
        assertTrue(guard.press(200));
        assertFalse(guard.press(300));
    }
}
