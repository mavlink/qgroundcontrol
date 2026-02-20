package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.lang.reflect.Method;
import java.util.List;

final class QGCStoragePermissionController {
    private static final String TAG = QGCStoragePermissionController.class.getSimpleName();
    static final int STORAGE_PERMISSION_REQUEST_CODE = 1;

    private final QGCActivity _activity;
    private volatile boolean _storagePermissionRequestInFlight = false;

    QGCStoragePermissionController(final QGCActivity activity) {
        _activity = activity;
    }

    String getSDCardPath() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            File[] appExternalDirs = _activity.getExternalFilesDirs(null);
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

        StorageManager storageManager = (StorageManager) _activity.getSystemService(Context.STORAGE_SERVICE);
        if (storageManager == null) {
            QGCLogger.w(TAG, "StorageManager unavailable");
            return "";
        }
        List<StorageVolume> volumes = storageManager.getStorageVolumes();

        for (StorageVolume vol : volumes) {
            if (!vol.isRemovable()) {
                continue;
            }

            String path;
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

    boolean checkStoragePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return true;
        }

        String[] permissions = {
            android.Manifest.permission.READ_EXTERNAL_STORAGE,
            android.Manifest.permission.WRITE_EXTERNAL_STORAGE
        };

        boolean allGranted = true;
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(_activity, permission) != PackageManager.PERMISSION_GRANTED) {
                allGranted = false;
                break;
            }
        }

        if (!allGranted) {
            if (!_storagePermissionRequestInFlight) {
                QGCLogger.i(TAG, "Storage permissions not granted, requesting...");
                _storagePermissionRequestInFlight = true;
                _activity.runOnUiThread(() -> ActivityCompat.requestPermissions(_activity, permissions, STORAGE_PERMISSION_REQUEST_CODE));
            } else {
                QGCLogger.d(TAG, "Storage permission request already in flight");
            }
            return false;
        }

        QGCLogger.i(TAG, "Storage permissions already granted");
        return true;
    }

    Boolean onRequestPermissionsResult(final int requestCode, final int[] grantResults) {
        if (requestCode != STORAGE_PERMISSION_REQUEST_CODE) {
            return null;
        }

        _storagePermissionRequestInFlight = false;
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

        return granted;
    }
}
