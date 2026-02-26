package org.mavlink.qgroundcontrol;

import android.app.Activity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLControllerManager;
import org.libsdl.app.HIDDeviceManager;

public class QGCSDLManager {
    private static final String TAG = QGCSDLManager.class.getSimpleName();

    private static HIDDeviceManager m_hidDeviceManager;

    public static void initialize(Activity activity) {
        try {
            System.loadLibrary("SDL3");

            SDL.setupJNI();
            SDL.initialize();
            SDL.setContext(activity);

            m_hidDeviceManager = HIDDeviceManager.acquire(activity);

            QGCLogger.i(TAG, "SDL initialized for joystick support");
        } catch (UnsatisfiedLinkError e) {
            QGCLogger.e(TAG, "SDL3 library not found", e);
        } catch (Exception e) {
            QGCLogger.e(TAG, "Failed to initialize SDL", e);
        }
    }

    public static void onPause() {
        if (m_hidDeviceManager != null) {
            m_hidDeviceManager.setFrozen(true);
        }
    }

    public static void onResume() {
        if (m_hidDeviceManager != null) {
            m_hidDeviceManager.setFrozen(false);
        }
    }

    public static void cleanup() {
        if (m_hidDeviceManager != null) {
            HIDDeviceManager.release(m_hidDeviceManager);
            m_hidDeviceManager = null;
        }
    }

    public static boolean handleMotionEvent(MotionEvent event) {
        if (isJoystickEvent(event)) {
            return SDLControllerManager.handleJoystickMotionEvent(event);
        }
        return false;
    }

    public static boolean handleKeyEvent(KeyEvent event) {
        if (!isJoystickButton(event)) {
            return false;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            return SDLControllerManager.onNativePadDown(event.getDeviceId(), event.getKeyCode());
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
            return SDLControllerManager.onNativePadUp(event.getDeviceId(), event.getKeyCode());
        }

        return false;
    }

    private static boolean isJoystickEvent(MotionEvent event) {
        int source = event.getSource();
        return (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
               (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD;
    }

    private static boolean isJoystickButton(KeyEvent event) {
        int source = event.getSource();
        if ((source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
            (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
            return true;
        }

        return KeyEvent.isGamepadButton(event.getKeyCode());
    }
}
