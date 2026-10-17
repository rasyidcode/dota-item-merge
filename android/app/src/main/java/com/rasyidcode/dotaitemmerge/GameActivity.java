package com.rasyidcode.dotaitemmerge;

import android.app.NativeActivity;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.Toast;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

/** Native game host that keeps Android's system bars hidden during play. */
public final class GameActivity extends NativeActivity {
    private final BackExitGuard backExitGuard = new BackExitGuard();
    private Toast exitToast;
    private OnBackInvokedCallback backCallback;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        hideSystemBars();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            backCallback = this::handleBackPress;
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    OnBackInvokedDispatcher.PRIORITY_DEFAULT, backCallback);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Wait for the decor view to attach before requesting its controller.
        getWindow().getDecorView().post(this::hideSystemBars);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemBars();
        }
    }

    @Override
    protected void onPause() {
        backExitGuard.reset();
        cancelExitToast();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && backCallback != null) {
            getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(backCallback);
        }
        cancelExitToast();
        super.onDestroy();
    }

    // Called by the native input queue on Android 6-12. UI work stays on the main thread.
    public void onNativeBackPressed() {
        runOnUiThread(this::handleBackPress);
    }

    private void handleBackPress() {
        if (!hasWindowFocus() || isFinishing() || isDestroyed()) {
            return;
        }
        if (backExitGuard.press(SystemClock.elapsedRealtime())) {
            cancelExitToast();
            // NativeActivity destruction lets the game save and release its world normally.
            finish();
        } else {
            cancelExitToast();
            exitToast = Toast.makeText(this, R.string.press_back_again_to_exit, Toast.LENGTH_SHORT);
            exitToast.show();
        }
    }

    private void cancelExitToast() {
        if (exitToast != null) {
            exitToast.cancel();
            exitToast = null;
        }
    }

    @SuppressWarnings("deprecation")
    private void hideSystemBars() {
        Window window = getWindow();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Transient bars overlay the game instead of resizing its surface.
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                controller.hide(WindowInsets.Type.systemBars());
            }
        } else {
            // Android 6-10 use the older sticky immersive API.
            window.getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }
}
