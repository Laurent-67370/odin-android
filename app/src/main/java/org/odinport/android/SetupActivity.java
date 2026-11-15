package org.odinport.android;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

/**
 * First screen: the folder Odin reads and writes (Odin/ of the shared
 * storage, all files allowed; else the app's own folder), Odin's data
 * unpacked from the APK, then Odin itself (OdinActivity).
 */
public class SetupActivity extends Activity {
    static final String PREFS = "odin";
    static final String KEY_FOLDER = "folder";
    static final String KEY_DATA = "data";
    private static final int ASK_STORAGE = 1;

    private boolean ownFolder;

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        ownFolder = getSharedPreferences(PREFS, MODE_PRIVATE).getBoolean("own_folder", false);
        if (hasAccess() || ownFolder) {
            start();
        } else {
            showAsk();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Back from the settings of Android with the access given
        if (!isFinishing() && hasAccess()) {
            start();
        }
    }

    private boolean hasAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        }
        return checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    private void askAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.parse("package:" + getPackageName())));
            } catch (Exception e) {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            }
        } else {
            requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE }, ASK_STORAGE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] permissions, int[] results) {
        if (code == ASK_STORAGE && hasAccess()) {
            start();
        }
    }

    private void showAsk() {
        final float dp = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, getResources().getDisplayMetrics());
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int pad = (int) (24 * dp);
        column.setPadding(pad, pad, pad, pad);

        TextView title = new TextView(this);
        title.setText("Odin");
        title.setTextColor(Color.rgb(0xD8, 0xAA, 0x4C));
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);
        title.setGravity(Gravity.CENTER);
        column.addView(title);

        TextView text = new TextView(this);
        text.setTextColor(Color.WHITE);
        text.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        text.setPadding(0, (int) (12 * dp), 0, (int) (12 * dp));
        text.setText("Odin lit vos jeux (archives MAME .zip / .7z) dans le dossier « Odin » de votre téléphone :\n\n"
                + "  Odin/roms     les jeux, avec hikaru.zip et mie.zip\n"
                + "  Odin/artwork  les images d'Odin-Artwork.zip (facultatif)\n\n"
                + "Pour cela, Android doit autoriser Odin à accéder à tous les fichiers. "
                + "Sans cette autorisation, Odin utilise son propre dossier :\n"
                + appFolder().getPath());
        column.addView(text);

        Button allow = new Button(this);
        allow.setText("Autoriser l'accès aux fichiers");
        allow.setOnClickListener(v -> askAccess());
        column.addView(allow);

        Button without = new Button(this);
        without.setText("Continuer sans (dossier de l'application)");
        without.setOnClickListener(v -> {
            ownFolder = true;
            getSharedPreferences(PREFS, MODE_PRIVATE).edit().putBoolean("own_folder", true).apply();
            start();
        });
        column.addView(without);

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.BLACK);
        scroll.addView(column);
        setContentView(scroll);
    }

    private File appFolder() {
        File own = getExternalFilesDir(null);
        return own != null ? own : getFilesDir();
    }

    private void start() {
        File folder = hasAccess() ? new File(Environment.getExternalStorageDirectory(), "Odin") : appFolder();
        File data = new File(getFilesDir(), "odin-data");

        for (String name : new String[] { "roms", "bios", "artwork" }) {
            new File(folder, name).mkdirs();
        }
        writeReadme(folder);
        try {
            unpackData(data);
        } catch (IOException e) {
            android.util.Log.e("Odin", "cannot unpack Odin's data", e);
        }

        SharedPreferences.Editor prefs = getSharedPreferences(PREFS, MODE_PRIVATE).edit();
        prefs.putString(KEY_FOLDER, folder.getPath());
        prefs.putString(KEY_DATA, data.getPath());
        prefs.commit();

        Intent intent = new Intent(this, OdinActivity.class);
        intent.putExtra(KEY_FOLDER, folder.getPath());
        intent.putExtra(KEY_DATA, data.getPath());
        startActivity(intent);
        finish();
    }

    private void writeReadme(File folder) {
        File readme = new File(folder, "LISEZMOI.txt");
        if (readme.exists()) {
            return;
        }
        String text = "Odin pour Android\n"
                + "=================\n\n"
                + "roms/     les jeux : archives MAME (.zip ou .7z), avec hikaru.zip et mie.zip\n"
                + "          pour les jeux SEGA Hikaru (jvs13551, 18wheelr et srally2 facultatifs)\n"
                + "bios/     les BIOS, si vous préférez les séparer des jeux\n"
                + "artwork/  les écrans-titres et décors d'Odin-Artwork.zip (facultatif)\n\n"
                + "odin.ini (réglages) et odin.log (journal) sont écrits ici.\n"
                + "Le bouton MENU de la manette tactile (ou la touche Retour) ouvre le menu d'Odin.\n";
        try (OutputStream out = new FileOutputStream(readme)) {
            out.write(text.getBytes(StandardCharsets.UTF_8));
        } catch (IOException ignored) {
        }
    }

    /** assets/odin/ copied to `data`, again when the APK changes */
    private void unpackData(File data) throws IOException {
        long version;
        try {
            version = getPackageManager().getPackageInfo(getPackageName(), 0).lastUpdateTime;
        } catch (PackageManager.NameNotFoundException e) {
            version = 0;
        }
        File stamp = new File(data, ".stamp");
        String wanted = Long.toString(version);
        if (stamp.exists() && wanted.equals(read(stamp))) {
            return;
        }
        data.mkdirs();
        try (InputStream list = getAssets().open("odin/files.txt");
             BufferedReader lines = new BufferedReader(new InputStreamReader(list, StandardCharsets.UTF_8))) {
            String line;
            byte[] buffer = new byte[1 << 16];
            while ((line = lines.readLine()) != null) {
                if (line.isEmpty()) {
                    continue;
                }
                File out = new File(data, line);
                out.getParentFile().mkdirs();
                try (InputStream in = getAssets().open("odin/" + line);
                     OutputStream to = new FileOutputStream(out)) {
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        to.write(buffer, 0, n);
                    }
                }
            }
        }
        try (OutputStream out = new FileOutputStream(stamp)) {
            out.write(wanted.getBytes(StandardCharsets.UTF_8));
        }
    }

    private static String read(File file) {
        try (InputStream in = new java.io.FileInputStream(file)) {
            java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
            byte[] buffer = new byte[256];
            int n;
            while ((n = in.read(buffer)) > 0) {
                bytes.write(buffer, 0, n);
            }
            return new String(bytes.toByteArray(), StandardCharsets.UTF_8);
        } catch (IOException e) {
            return "";
        }
    }
}
