package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.provider.Settings;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.lang.reflect.Method;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

final class QGCStoragePermissionController {
    private static final String TAG = QGCStoragePermissionController.class.getSimpleName();
    static final int STORAGE_PERMISSION_REQUEST_CODE = 1;

    private final QGCActivity _activity;
    private volatile boolean _storagePermissionRequestInFlight = false;
    private final AtomicBoolean _allFilesAccessRequestInFlight = new AtomicBoolean(false);
    private final AtomicBoolean _pausedSinceAllFilesAccessRequest = new AtomicBoolean(false);

    QGCStoragePermissionController(final QGCActivity activity) {
        _activity = activity;
    }

    static boolean requiresRuntimeStoragePermission(final int sdkInt) {
        return sdkInt < Build.VERSION_CODES.R;
    }

    // All-files access (MANAGE_EXTERNAL_STORAGE) applies only on API 30+ builds whose manifest declares it
    private boolean usesAllFilesAccess() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && manifestDeclaresManageStorage();
    }

    private static boolean isExternalStorageManager() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && Environment.isExternalStorageManager();
    }

    private boolean manifestDeclaresManageStorage() {
        try {
            final PackageInfo info = _activity.getPackageManager()
                .getPackageInfo(_activity.getPackageName(), PackageManager.GET_PERMISSIONS);
            if (info.requestedPermissions != null) {
                for (String permission : info.requestedPermissions) {
                    if (android.Manifest.permission.MANAGE_EXTERNAL_STORAGE.equals(permission)) {
                        return true;
                    }
                }
            }
        } catch (Exception e) {
            QGCLogger.e(TAG, "Failed to read manifest permissions", e);
        }
        return false;
    }

    static boolean areAllPermissionsGranted(final int[] grantResults) {
        if (grantResults == null || grantResults.length == 0) {
            return false;
        }

        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }

        return true;
    }

    void setStoragePermissionRequestInFlightForTesting(final boolean inFlight) {
        _storagePermissionRequestInFlight = inFlight;
    }

    boolean isStoragePermissionRequestInFlightForTesting() {
        return _storagePermissionRequestInFlight;
    }

    String getSDCardPath() {
        if (usesAllFilesAccess()) {
            if (!isExternalStorageManager()) {
                QGCLogger.w(TAG, "All files access not granted");
                return "";
            }

            final StorageManager storageManager = (StorageManager) _activity.getSystemService(Context.STORAGE_SERVICE);
            if (storageManager == null) {
                QGCLogger.w(TAG, "StorageManager unavailable");
                return "";
            }

            for (StorageVolume vol : storageManager.getStorageVolumes()) {
                if (!vol.isRemovable()) {
                    continue;
                }

                final File dir = vol.getDirectory();
                if (dir != null) {
                    final String path = dir.getAbsolutePath();
                    QGCLogger.i(TAG, "removable sd card root at " + path);
                    return path;
                }
            }

            QGCLogger.w(TAG, "No removable SD card found");
            return "";
        }

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

            // Pre-API 30: getPath() is hidden, use reflection
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
        if (!requiresRuntimeStoragePermission(Build.VERSION.SDK_INT)) {
            if (!usesAllFilesAccess()) {
                // Scoped storage: app-specific SD card directory needs no permission
                return true;
            }

            if (isExternalStorageManager()) {
                QGCLogger.i(TAG, "All files access already granted");
                return true;
            }

            if (_allFilesAccessRequestInFlight.compareAndSet(false, true)) {
                QGCLogger.i(TAG, "All files access not granted, opening system settings...");
                _pausedSinceAllFilesAccessRequest.set(false);
                _activity.runOnUiThread(this::_launchAllFilesAccessSettings);
            } else {
                QGCLogger.d(TAG, "All files access request already in flight");
            }
            return false;
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
        final boolean granted = areAllPermissionsGranted(grantResults);

        if (granted) {
            QGCLogger.i(TAG, "Storage permissions granted via runtime prompt");
        } else {
            QGCLogger.w(TAG, "Storage permissions denied via runtime prompt");
        }

        return granted;
    }

    private void _launchAllFilesAccessSettings() {
        // Note: the settings screen opens in the Settings app's own task, so startActivityForResult
        // would deliver an immediate RESULT_CANCELED. The outcome is instead evaluated in onResume().
        try {
            final Intent intent = new Intent(
                Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                Uri.fromParts("package", _activity.getPackageName(), null));
            _activity.startActivity(intent);
        } catch (Exception e) {
            QGCLogger.w(TAG, "Failed to open app all files access settings, trying global settings", e);
            try {
                final Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                _activity.startActivity(intent);
            } catch (Exception e2) {
                QGCLogger.e(TAG, "Failed to open all files access settings", e2);
                _allFilesAccessRequestInFlight.set(false);
            }
        }
    }

    void onPause() {
        if (_allFilesAccessRequestInFlight.get()) {
            _pausedSinceAllFilesAccessRequest.set(true);
        }
    }

    /// Returns the grant decision when returning from the all files access settings screen, null otherwise
    Boolean onResume() {
        // compareAndSet atomically consumes the in-flight request so the result is delivered exactly once
        if (!_pausedSinceAllFilesAccessRequest.get() || !_allFilesAccessRequestInFlight.compareAndSet(true, false)) {
            return null;
        }

        final boolean granted = isExternalStorageManager();

        if (granted) {
            QGCLogger.i(TAG, "All files access granted via system settings");
        } else {
            QGCLogger.w(TAG, "All files access denied via system settings");
        }

        return granted;
    }
}
