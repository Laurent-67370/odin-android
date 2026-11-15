package org.odinport.android;

import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.view.InputDevice;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

import org.libsdl.app.SDLActivity;

/**
 * Odin itself: SDL's activity running SDL_main of libmain.so, told its
 * folders through the environment (user_files.cpp of the patch).
 */
public class OdinActivity extends SDLActivity {
    @Override
    protected void onCreate(Bundle saved) {
        SharedPreferences prefs = getSharedPreferences(SetupActivity.PREFS, MODE_PRIVATE);
        String folder = getIntent().getStringExtra(SetupActivity.KEY_FOLDER);
        String data = getIntent().getStringExtra(SetupActivity.KEY_DATA);
        if (folder == null) {
            folder = prefs.getString(SetupActivity.KEY_FOLDER, getFilesDir().getPath());
        }
        if (data == null) {
            data = prefs.getString(SetupActivity.KEY_DATA, getFilesDir().getPath() + "/odin-data");
        }
        try {
            Os.setenv("ODIN_ANDROID_FOLDER", folder, true);
            Os.setenv("ODIN_ANDROID_DATA", data, true);
            Os.setenv("HOME", folder, true);
            // Measures in odin.log while the port is tuned: the speed every
            // second (ODIN_SPEED), and where the time went when a game is
            // closed (ODIN_STATS)
            Os.setenv("ODIN_SPEED", "1", false);
            Os.setenv("ODIN_STATS", "1", false);
            // The cost of each part of the drawing, on the thread and on the
            // graphics card, every 300 frames
            Os.setenv("ODIN_DRAW_TIMES", "1", false);
        } catch (ErrnoException e) {
            android.util.Log.e("Odin", "setenv", e);
        }
        super.onCreate(saved);
        hideBars();
        touchAtOnce();
    }

    /** The touches given to the game as they come, not gathered until the
     *  next frame of the screen (up to 16 ms sooner) */
    private void touchAtOnce() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mSurface != null) {
            mSurface.requestUnbufferedDispatch(InputDevice.SOURCE_TOUCHSCREEN);
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "c++_shared", "SDL3", "main" };
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideBars();
        touchAtOnce();
    }

    @Override
    public void onWindowFocusChanged(boolean focus) {
        super.onWindowFocusChanged(focus);
        if (focus) {
            hideBars();
        }
    }

    /** The whole screen for the game; the bars come back with a swipe */
    private void hideBars() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Odin's native state is made for one run: the next start is a new process
        if (isFinishing()) {
            System.exit(0);
        }
    }
}
