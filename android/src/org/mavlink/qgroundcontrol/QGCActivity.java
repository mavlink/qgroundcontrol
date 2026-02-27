package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity {
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String MULTICAST_LOCK_TAG = "QGroundControl";

    private static volatile QGCActivity m_instance = null;

    private WifiManager.MulticastLock m_wifiMulticastLock;
    private volatile QGCStoragePermissionController m_storagePermissionController;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        m_instance = this;

        nativeInit();
        setupMulticastLock();

        QGCUsbSerialManager.initialize(this);
        QGCSDLManager.initialize(this);
        m_storagePermissionController = new QGCStoragePermissionController(this);
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
        if (activity.m_storagePermissionController == null) {
            activity.m_storagePermissionController = new QGCStoragePermissionController(activity);
        }
        return activity.m_storagePermissionController.getSDCardPath();
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
        if (activity.m_storagePermissionController == null) {
            activity.m_storagePermissionController = new QGCStoragePermissionController(activity);
        }
        return activity.m_storagePermissionController.checkStoragePermissions();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (m_storagePermissionController == null) {
            m_storagePermissionController = new QGCStoragePermissionController(this);
        }

        final Boolean granted = m_storagePermissionController.onRequestPermissionsResult(requestCode, grantResults);
        if (granted == null) {
            return;
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
    public native void nativeStoragePermissionsResult(boolean granted);
}
