package org.libsdl.app;

import android.app.Activity;

/**
 * SDL library initialization - minimal stub for QGroundControl
 * Only joystick/gamepad functionality is used; video/audio are handled by Qt.
 */
public class SDL {

    protected static Activity mContext;

    public static void setupJNI() {
        SDLActivity.nativeSetupJNI();
        SDLControllerManager.nativeSetupJNI();
    }

    public static void initialize() {
        setContext(null);

        SDLControllerManager.initialize();
    }

    public static void setContext(Activity context) {
        mContext = context;
    }

    public static Activity getContext() {
        return mContext;
    }

    public static void loadLibrary(String libraryName) throws UnsatisfiedLinkError, SecurityException, NullPointerException {
        if (libraryName == null) {
            throw new NullPointerException("No library name provided.");
        }
        System.loadLibrary(libraryName);
    }
}
