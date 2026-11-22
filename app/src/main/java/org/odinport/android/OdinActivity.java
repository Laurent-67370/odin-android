package org.odinport.android;

import android.app.AlertDialog;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.view.InputDevice;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.libsdl.app.SDLActivity;

/**
 * Odin itself: SDL's activity running SDL_main of libmain.so, told its
 * folders through the environment (user_files.cpp of the patch).
 */
public class OdinActivity extends SDLActivity {
    /** The homebrew games published with the APK (build.py writes it) */
    static final String HOMEBREW_LIST =
            "https://github.com/Laurent-67370/odin-android/releases/download/odin-apk/homebrew.json";

    /** The folder of the games, Odin/roms */
    private String mRoms;

    /** Odin's answer to askHomebrew (folder_dialog.cpp): the files downloaded,
     *  file:// addresses one a line, or none, or why there are none */
    private static native void homebrewChosen(String files, String error);

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
        mRoms = folder + "/roms";
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

    /**
     * Asked by Odin (MENU, File, Get homebrew games...), on its own thread:
     * the list of the homebrew games fetched, each shown new, installed or
     * with an update (the CRC of its first EPROM against the one in
     * Odin/roms), those chosen downloaded to the cache; Odin adds them as
     * it adds archives (import_games.cpp).
     */
    public void askHomebrew() {
        new Thread(() -> {
            try {
                JSONArray games = new JSONObject(new String(fetch(HOMEBREW_LIST), StandardCharsets.UTF_8))
                        .getJSONArray("games");
                runOnUiThread(() -> chooseHomebrew(games));
            } catch (Exception e) {
                android.util.Log.e("Odin", "homebrew list", e);
                homebrewChosen("", "No list of homebrew games (no network?): " + e.getMessage());
            }
        }, "homebrew list").start();
    }

    private void chooseHomebrew(JSONArray games) {
        int count = games.length();
        String[] labels = new String[count];
        boolean[] chosen = new boolean[count];
        List<JSONObject> list = new ArrayList<>();

        for (int i = 0; i < count; i++) {
            JSONObject game = games.optJSONObject(i);
            String name = game.optString("name"), crc = game.optString("crc");
            String now = crcOf(new File(mRoms, name + ".zip"), name + ".ic29");
            String status = now == null ? "new" : now.equalsIgnoreCase(crc) ? "installed" : "update";

            list.add(game);
            labels[i] = game.optString("title") + " - " + game.optString("genre") + " (" + status + ")";
            chosen[i] = !status.equals("installed");
        }
        final boolean[] answered = { false };
        new AlertDialog.Builder(this)
                .setTitle("Homebrew games")
                .setMultiChoiceItems(labels, chosen, (dialog, which, on) -> chosen[which] = on)
                .setPositiveButton("Install", (dialog, which) -> {
                    answered[0] = true;
                    List<JSONObject> wanted = new ArrayList<>();
                    for (int i = 0; i < count; i++) {
                        if (chosen[i]) {
                            wanted.add(list.get(i));
                        }
                    }
                    download(wanted);
                })
                .setNegativeButton("Cancel", (dialog, which) -> {
                    answered[0] = true;
                    homebrewChosen("", null);
                })
                .setOnDismissListener(dialog -> {
                    if (!answered[0]) {
                        homebrewChosen("", null);
                    }
                    hideBars();
                })
                .show();
    }

    private void download(List<JSONObject> games) {
        if (games.isEmpty()) {
            homebrewChosen("", null);
            return;
        }
        Toast.makeText(this, "Downloading " + games.size() + " homebrew game" + (games.size() > 1 ? "s" : "") + "...",
                Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            StringBuilder files = new StringBuilder();
            String error = null;
            File folder = new File(getCacheDir(), "homebrew");

            folder.mkdirs();
            for (JSONObject game : games) {
                File file = new File(folder, game.optString("name") + ".zip");
                try (OutputStream out = new FileOutputStream(file)) {
                    out.write(fetch(game.optString("url")));
                    files.append("file://").append(file.getPath()).append('\n');
                } catch (Exception e) {
                    android.util.Log.e("Odin", "homebrew download", e);
                    error = "Cannot download " + game.optString("title") + ": " + e.getMessage();
                }
            }
            homebrewChosen(files.toString(), files.length() == 0 ? error : null);
        }, "homebrew download").start();
    }

    /** The CRC of an entry of a zip, as 8 hex digits; null when there is none */
    private static String crcOf(File zip, String entry) {
        if (!zip.isFile()) {
            return null;
        }
        try (ZipFile file = new ZipFile(zip)) {
            ZipEntry found = file.getEntry(entry);
            return found == null ? null : String.format("%08x", found.getCrc());
        } catch (Exception e) {
            return null;
        }
    }

    /** What an address of the web holds (GitHub's downloads go on elsewhere) */
    private static byte[] fetch(String address) throws Exception {
        for (int hops = 0; hops < 5; hops++) {
            HttpURLConnection connection = (HttpURLConnection) new URL(address).openConnection();
            connection.setConnectTimeout(15000);
            connection.setReadTimeout(30000);
            connection.setInstanceFollowRedirects(false);
            int code = connection.getResponseCode();
            if (code >= 300 && code < 400 && connection.getHeaderField("Location") != null) {
                address = new URL(new URL(address), connection.getHeaderField("Location")).toString();
                connection.disconnect();
                continue;
            }
            if (code != 200) {
                connection.disconnect();
                throw new Exception("HTTP " + code);
            }
            try (InputStream in = connection.getInputStream()) {
                ByteArrayOutputStream out = new ByteArrayOutputStream();
                byte[] buffer = new byte[65536];
                for (int got; (got = in.read(buffer)) > 0;) {
                    out.write(buffer, 0, got);
                }
                return out.toByteArray();
            } finally {
                connection.disconnect();
            }
        }
        throw new Exception("too many redirections");
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
