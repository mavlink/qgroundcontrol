/**
 * Copy this file into your Android project and call init(). If your project
 * contains fonts and/or certificates in assets, uncomment copyFonts() and/or
 * copyCaCertificates() lines in init().
 */
package org.freedesktop.gstreamer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.content.res.AssetManager;
import android.system.Os;

public class GStreamer {
    private static native void nativeInit(Context context) throws Exception;

    public static void init(Context context) throws Exception {
        //copyFonts(context);
        //copyCaCertificates(context);
        nativeInit(context);
    }

    private static void copyFonts(Context context) {
        AssetManager assetManager = context.getAssets();
        File filesDir = context.getFilesDir();
        File fontsFCDir = new File (filesDir, "fontconfig");
        File fontsDir = new File (fontsFCDir, "fonts");
        File fontsCfg = new File (fontsFCDir, "fonts.conf");

        fontsDir.mkdirs();

        try {
            /* Copy the config file */
            copyFile (assetManager, "fontconfig/fonts.conf", fontsCfg);
            /* Copy the fonts */
            for(String filename : assetManager.list("fontconfig/fonts/truetype")) {
                File font = new File(fontsDir, filename);
                copyFile (assetManager, "fontconfig/fonts/truetype/" + filename, font);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void copyCaCertificates(Context context) {
        AssetManager assetManager = context.getAssets();
        File filesDir = context.getFilesDir();
        File sslDir = new File (filesDir, "ssl");
        File certsDir = new File (sslDir, "certs");
        File certs = new File (certsDir, "ca-certificates.crt");

        certsDir.mkdirs();

        try {
            /* Copy the certificates file */
            copyFile (assetManager, "ssl/certs/ca-certificates.crt", certs);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void copyFile(AssetManager assetManager, String assetPath, File outFile) throws IOException {
        InputStream in = null;
        OutputStream out = null;
        IOException exception = null;

        if (outFile.exists())
            outFile.delete();

        try {
            in = assetManager.open(assetPath);
            out = new FileOutputStream(outFile);

            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            out.flush();
        } catch (IOException e) {
            exception = e;
        } finally {
            if (in != null)
                try {
                    in.close();
                } catch (IOException e) {
                    if (exception == null)
                        exception = e;
                }
            if (out != null)
                try {
                    out.close();
                } catch (IOException e) {
                    if (exception == null)
                        exception = e;
                }
            if (exception != null)
                throw exception;
        }
    }
}
