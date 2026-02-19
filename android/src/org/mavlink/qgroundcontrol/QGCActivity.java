package org.mavlink.qgroundcontrol;

import java.io.File;
import java.util.List;
import java.lang.reflect.Method;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;

import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.view.KeyEvent;
import android.view.MotionEvent;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity {
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String MULTICAST_LOCK_TAG = "QGroundControl";
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 1;

    private static volatile QGCActivity m_instance = null;
    private static volatile boolean s_storagePermissionRequestInFlight = false;

    private WifiManager.MulticastLock m_wifiMulticastLock;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        m_instance = this;

        nativeInit();
        setupMulticastLock();

        QGCUsbSerialManager.initialize(this);
        QGCSDLManager.initialize(this);
    }

    @Override
    protected void onPause() {
        QGCSDLManager.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        QGCSDLManager.onResume();
    }

    @Override
    protected void onDestroy() {
        try {
            QGCSDLManager.cleanup();
            releaseMulticastLock();
            QGCUsbSerialManager.cleanup(this);
        } catch (final Exception e) {
            QGCLogger.e(TAG, "Exception onDestroy()", e);
        }

        if (m_instance == this) {
            m_instance = null;
        }
        super.onDestroy();
    }

    /**
     * Sets up a multicast lock to allow multicast packets.
     */
    private void setupMulticastLock() {
        if (m_wifiMulticastLock == null) {
            final WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
            if (wifi == null) {
                QGCLogger.w(TAG, "WifiManager is unavailable; multicast lock not acquired");
                return;
            }
            m_wifiMulticastLock = wifi.createMulticastLock(MULTICAST_LOCK_TAG);
            m_wifiMulticastLock.setReferenceCounted(false);
        }

        if (m_wifiMulticastLock == null) {
            return;
        }
        m_wifiMulticastLock.acquire();
        QGCLogger.d(TAG, "Multicast lock: " + m_wifiMulticastLock.toString());
    }

    /**
     * Releases the multicast lock if held.
     */
    private void releaseMulticastLock() {
        if (m_wifiMulticastLock != null && m_wifiMulticastLock.isHeld()) {
            m_wifiMulticastLock.release();
            QGCLogger.d(TAG, "Multicast lock released.");
        }
    }

    public static String getSDCardPath() {
        final QGCActivity activity = m_instance;
        if (activity == null) {
            QGCLogger.e(TAG, "Activity instance is null");
            return "";
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            File[] appExternalDirs = activity.getExternalFilesDirs(null);
            if (appExternalDirs != null) {
                for (File dir : appExternalDirs) {
                    if (dir == null || !Environment.isExternalStorageRemovable(dir)) {
                        continue;
                    }

                    final String path = dir.getAbsolutePath();
                    if (!path.isEmpty()) {
                        QGCLogger.i(TAG, "removable sd card app directory at " + path);
                        return path;
                    }
                }
            }

            QGCLogger.w(TAG, "No removable SD card app directory found");
            return "";
        }

        StorageManager storageManager = (StorageManager) activity.getSystemService(Context.STORAGE_SERVICE);
        if (storageManager == null) {
            QGCLogger.w(TAG, "StorageManager unavailable");
            return "";
        }
        List<StorageVolume> volumes = storageManager.getStorageVolumes();

        for (StorageVolume vol : volumes) {
            if (!vol.isRemovable()) {
                continue;
            }

            String path = null;

            // For older versions, use reflection to get the path.
            try {
                Method mMethodGetPath = vol.getClass().getMethod("getPath");
                path = (String) mMethodGetPath.invoke(vol);
            } catch (Exception e) {
                QGCLogger.e(TAG, "Failed to get path via reflection", e);
                continue;
            }

            if (path != null && !path.isEmpty()) {
                QGCLogger.i(TAG, "removable sd card mounted at " + path);
                return path;
            }
        }

        QGCLogger.w(TAG, "No removable SD card found");
        return "";
    }

    /**
     * Checks and requests storage permissions for SD card access.
     * Android 11+ uses app-scoped storage and does not require runtime storage
     * permissions for app-owned directories.
     *
     * @return true if permissions are granted, false otherwise
     */
    public static boolean checkStoragePermissions() {
        final QGCActivity activity = m_instance;
        if (activity == null) {
            QGCLogger.e(TAG, "Activity instance is null");
            return false;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // App-scoped directories on external/removable storage are writable without
            // legacy all-files permissions on Android 11+.
            return true;
        } else {
            // Android 9-10 (API 28-29) uses runtime storage permissions.
            String[] permissions = {
                android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE
            };

            boolean allGranted = true;
            for (String permission : permissions) {
                if (ContextCompat.checkSelfPermission(activity, permission) != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            if (!allGranted) {
                if (!s_storagePermissionRequestInFlight) {
                    QGCLogger.i(TAG, "Storage permissions not granted, requesting...");
                    s_storagePermissionRequestInFlight = true;
                    activity.runOnUiThread(() -> ActivityCompat.requestPermissions(activity, permissions, STORAGE_PERMISSION_REQUEST_CODE));
                } else {
                    QGCLogger.d(TAG, "Storage permission request already in flight");
                }
                return false;
            }

            QGCLogger.i(TAG, "Storage permissions already granted");
            return true;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode != STORAGE_PERMISSION_REQUEST_CODE) {
            return;
        }

        s_storagePermissionRequestInFlight = false;
        boolean granted = grantResults.length > 0;
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                granted = false;
                break;
            }
        }

        if (granted) {
            QGCLogger.i(TAG, "Storage permissions granted via runtime prompt");
        } else {
            QGCLogger.w(TAG, "Storage permissions denied via runtime prompt");
        }

        nativeStoragePermissionsResult(granted);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if (QGCSDLManager.handleMotionEvent(event)) return true;
        return super.dispatchGenericMotionEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (QGCSDLManager.handleKeyEvent(event)) return true;
        return super.dispatchKeyEvent(event);
    }

    // Native C++ functions
    public native boolean nativeInit();
    public native void qgcLogDebug(final String message);
    public native void qgcLogWarning(final String message);
    public native void nativeStoragePermissionsResult(boolean granted);
}
