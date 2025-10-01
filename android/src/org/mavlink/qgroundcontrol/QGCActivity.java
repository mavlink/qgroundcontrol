package org.mavlink.qgroundcontrol;

import java.io.File;
import java.util.List;
import java.lang.reflect.Method;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.net.wifi.WifiManager;
import android.provider.Settings;
import android.util.Log;
import android.view.WindowManager;
import android.app.Activity;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity {
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String SCREEN_BRIGHT_WAKE_LOCK_TAG = "QGroundControl";
    private static final String MULTICAST_LOCK_TAG = "QGroundControl";

    private static QGCActivity m_instance = null;

    private PowerManager.WakeLock m_wakeLock;
    private WifiManager.MulticastLock m_wifiMulticastLock;

    public QGCActivity() {
        m_instance = this;
    }

    /**
     * Returns the singleton instance of QGCActivity.
     *
     * @return The current instance of QGCActivity.
     */
    public static QGCActivity getInstance() {
        return m_instance;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeInit();
        acquireWakeLock();
        keepScreenOn();
        setupMulticastLock();

        QGCUsbSerialManager.initialize(this);
    }

    @Override
    protected void onDestroy() {
        try {
            releaseMulticastLock();
            releaseWakeLock();
            QGCUsbSerialManager.cleanup(this);
        } catch (final Exception e) {
            Log.e(TAG, "Exception onDestroy()", e);
        }

        super.onDestroy();
    }

    /**
     * Keeps the screen on by adding the appropriate window flag.
     */
    private void keepScreenOn() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    /**
     * Acquires a wake lock to keep the CPU running.
     */
    private void acquireWakeLock() {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        m_wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, SCREEN_BRIGHT_WAKE_LOCK_TAG);
        if (m_wakeLock != null) {
            m_wakeLock.acquire();
        } else {
            Log.w(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!");
        }
    }

    /**
     * Releases the wake lock if held.
     */
    private void releaseWakeLock() {
        if (m_wakeLock != null && m_wakeLock.isHeld()) {
            m_wakeLock.release();
        }
    }

    /**
     * Sets up a multicast lock to allow multicast packets.
     */
    private void setupMulticastLock() {
        if (m_wifiMulticastLock == null) {
            final WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
            m_wifiMulticastLock = wifi.createMulticastLock(MULTICAST_LOCK_TAG);
            m_wifiMulticastLock.setReferenceCounted(true);
        }

        m_wifiMulticastLock.acquire();
        Log.d(TAG, "Multicast lock: " + m_wifiMulticastLock.toString());
    }

    /**
     * Releases the multicast lock if held.
     */
    private void releaseMulticastLock() {
        if (m_wifiMulticastLock != null && m_wifiMulticastLock.isHeld()) {
            m_wifiMulticastLock.release();
            Log.d(TAG, "Multicast lock released.");
        }
    }

    public static String getSDCardPath() {
        StorageManager storageManager = (StorageManager)m_instance.getSystemService(Activity.STORAGE_SERVICE);
        List<StorageVolume> volumes = storageManager.getStorageVolumes();
        
        for (StorageVolume vol : volumes) {
            if (!vol.isRemovable()) {
                continue;
            }
            
            String path = null;
            
            // For Android 11+ (API 30+), use the proper getDirectory() method
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                File directory = vol.getDirectory();
                if (directory != null) {
                    path = directory.getAbsolutePath();
                }
            } else {
                // For older versions, use reflection to get the path
                try {
                    Method mMethodGetPath = vol.getClass().getMethod("getPath");
                    path = (String) mMethodGetPath.invoke(vol);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to get path via reflection", e);
                    continue;
                }
            }
            
            if (path != null && !path.isEmpty()) {
                Log.i(TAG, "removable sd card mounted at " + path);
                return path;
            }
        }
        
        Log.w(TAG, "No removable SD card found");
        return "";
    }

    /**
     * Checks and requests storage permissions for SD card access.
     * For Android 11+ (API 30+), this requires MANAGE_EXTERNAL_STORAGE permission.
     *
     * @return true if permissions are granted, false otherwise
     */
    public static boolean checkStoragePermissions() {
        if (m_instance == null) {
            Log.e(TAG, "Activity instance is null");
            return false;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+) requires MANAGE_EXTERNAL_STORAGE for full SD card access
            if (!Environment.isExternalStorageManager()) {
                Log.i(TAG, "MANAGE_EXTERNAL_STORAGE not granted, requesting...");
                try {
                    Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                    intent.setData(Uri.parse("package:" + m_instance.getPackageName()));
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    m_instance.startActivity(intent);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to open storage permission settings", e);
                    // Fallback to general settings
                    Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    m_instance.startActivity(intent);
                }
                return false;
            }
            Log.i(TAG, "MANAGE_EXTERNAL_STORAGE already granted");
            return true;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Android 6.0+ (API 23+) requires runtime permissions
            String[] permissions = {
                android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE
            };

            boolean allGranted = true;
            for (String permission : permissions) {
                if (ContextCompat.checkSelfPermission(m_instance, permission) != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            if (!allGranted) {
                Log.i(TAG, "Storage permissions not granted, requesting...");
                ActivityCompat.requestPermissions(m_instance, permissions, 1);
                return false;
            }

            Log.i(TAG, "Storage permissions already granted");
            return true;
        } else {
            // Below Android 6.0, permissions are granted at install time
            return true;
        }
    }

    // Native C++ functions
    public native boolean nativeInit();
    public native void qgcLogDebug(final String message);
    public native void qgcLogWarning(final String message);
}
