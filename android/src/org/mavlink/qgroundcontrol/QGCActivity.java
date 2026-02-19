package org.mavlink.qgroundcontrol;

import java.io.File;
import java.util.List;
import java.lang.reflect.Method;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.qtproject.qt.android.bindings.QtActivity;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLControllerManager;
import org.libsdl.app.HIDDeviceManager;

import org.freedesktop.gstreamer.GStreamer;

public class QGCActivity extends QtActivity {
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String SCREEN_BRIGHT_WAKE_LOCK_TAG = "QGroundControl";
    private static final String MULTICAST_LOCK_TAG = "QGroundControl";

    private static QGCActivity m_instance = null;

    private PowerManager.WakeLock m_wakeLock;
    private WifiManager.MulticastLock m_wifiMulticastLock;
    private HIDDeviceManager m_hidDeviceManager;

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

        final boolean nativeInitSucceeded = nativeInit();
        initializeGStreamer(nativeInitSucceeded);
        acquireWakeLock();
        keepScreenOn();
        setupMulticastLock();

        QGCUsbSerialManager.initialize(this);

        // Initialize SDL for joystick support
        initializeSDL();
    }

    private void initializeGStreamer(boolean nativeInitSucceeded) {
        if (!nativeInitSucceeded) {
            nativeGstInitResult(false);
            Log.e(TAG, "nativeInit failed; skipping GStreamer initialization");
            return;
        }

        try {
            System.loadLibrary("gstreamer_android");
            GStreamer.init(this);
            nativeGstInitResult(true);
            Log.i(TAG, "GStreamer initialized successfully");
        } catch (UnsatisfiedLinkError e) {
            nativeGstInitResult(false);
            Log.e(TAG, "GStreamer library not found: " + e.getMessage());
        } catch (Exception e) {
            nativeGstInitResult(false);
            Log.e(TAG, "Failed to initialize GStreamer: " + e.getMessage());
        }
    }

    /**
     * Initializes SDL for joystick/gamepad support.
     * SDL handles controller input through its Java layer (SDLControllerManager)
     * which communicates with the native SDL library.
     */
    private void initializeSDL() {
        try {
            // Load the SDL shared library - this triggers SDL's JNI_OnLoad
            System.loadLibrary("SDL3");

            // Setup JNI bindings and initialize controller manager
            SDL.setupJNI();
            SDL.initialize();

            // Set SDL context to this activity AFTER initialize()
            // (initialize() calls setContext(null) to clear previous state)
            SDL.setContext(this);

            // Acquire HIDDeviceManager for USB HID and Bluetooth controller support
            m_hidDeviceManager = HIDDeviceManager.acquire(this);

            Log.i(TAG, "SDL initialized for joystick support");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "SDL3 library not found: " + e.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize SDL: " + e.getMessage());
        }
    }

    @Override
    protected void onPause() {
        if (m_hidDeviceManager != null) {
            m_hidDeviceManager.setFrozen(true);
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (m_hidDeviceManager != null) {
            m_hidDeviceManager.setFrozen(false);
        }
    }

    @Override
    protected void onDestroy() {
        try {
            if (m_hidDeviceManager != null) {
                HIDDeviceManager.release(m_hidDeviceManager);
                m_hidDeviceManager = null;
            }
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
     * Checks storage permissions for SD card access.
     *
     * @return true if permissions are granted, false otherwise
     */
    public static boolean checkStoragePermissions() {
        if (m_instance == null) {
            Log.e(TAG, "Activity instance is null");
            return false;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+) requires MANAGE_EXTERNAL_STORAGE for full SD card access.
            // Do not launch settings from here; this path is called during startup.
            if (!Environment.isExternalStorageManager()) {
                Log.i(TAG, "MANAGE_EXTERNAL_STORAGE not granted");
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

    // =========================================================================
    // Input Event Forwarding to SDL
    // =========================================================================

    /**
     * Forward joystick/gamepad motion events to SDL
     */
    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if (isJoystickEvent(event)) {
            if (SDLControllerManager.handleJoystickMotionEvent(event)) {
                return true;
            }
        }
        return super.dispatchGenericMotionEvent(event);
    }

    /**
     * Forward joystick/gamepad key events to SDL
     */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (isJoystickButton(event)) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (SDLControllerManager.onNativePadDown(event.getDeviceId(), event.getKeyCode())) {
                    return true;
                }
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                if (SDLControllerManager.onNativePadUp(event.getDeviceId(), event.getKeyCode())) {
                    return true;
                }
            }
        }
        return super.dispatchKeyEvent(event);
    }

    /**
     * Check if the motion event is from a joystick
     */
    private boolean isJoystickEvent(MotionEvent event) {
        int source = event.getSource();
        return (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
               (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD;
    }

    /**
     * Check if the key event is a joystick button
     */
    private boolean isJoystickButton(KeyEvent event) {
        int source = event.getSource();
        if ((source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
            (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
            return true;
        }

        // Also check for known gamepad buttons
        int keyCode = event.getKeyCode();
        return KeyEvent.isGamepadButton(keyCode);
    }

    // Native C++ functions
    public native boolean nativeInit();
    public native void nativeGstInitResult(boolean success);
    public native void qgcLogDebug(final String message);
    public native void qgcLogWarning(final String message);
}
