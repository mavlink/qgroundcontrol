package org.libsdl.app;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.content.res.Configuration;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.View;
import android.view.inputmethod.InputConnection;

/**
 * SDLActivity stub for QGroundControl
 *
 * QGC uses SDL only for joystick/gamepad support - Qt handles everything else
 * (rendering, audio, input, lifecycle). This class provides the minimal interface
 * required by SDL's JNI layer.
 *
 * Methods fall into three categories:
 * 1. Native method declarations - Required for SDL's JNI registration
 * 2. Platform detection methods - Actually used by QGC via SDLHelper
 * 3. Stub methods - Called by SDL but return safe no-op values
 */
public class SDLActivity {

    // =========================================================================
    // Native Method Declarations
    // These MUST exist for SDL's JNI_OnLoad to register native implementations.
    // We never call most of these - they exist only for JNI linkage.
    // =========================================================================

    // Core lifecycle
    public static native String nativeGetVersion();
    public static native void nativeSetupJNI();
    public static native void nativeInitMainThread();
    public static native void nativeCleanupMainThread();
    public static native int nativeRunMain(String library, String function, Object arguments);
    public static native void nativeLowMemory();
    public static native void nativeSendQuit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void nativeFocusChanged(boolean hasFocus);
    public static native boolean nativeAllowRecreateActivity();
    public static native int nativeCheckSDLThreadCounter();

    // Input events
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native boolean onNativeSoftReturnKey();
    public static native void onNativeKeyboardFocusLost();
    public static native void onNativeMouse(int button, int action, float x, float y, boolean relative);
    public static native void onNativeTouch(int touchDevId, int pointerFingerId, int action, float x, float y, float p);
    public static native void onNativePen(int penId, int device_type, int button, int action, float x, float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void onNativeDropFile(String filename);
    public static native void nativeAddTouch(int touchId, String name);

    // Display/surface
    public static native void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight, int deviceWidth, int deviceHeight, float density, float rate);
    public static native void onNativeResize();
    public static native void onNativeSurfaceCreated();
    public static native void onNativeSurfaceChanged();
    public static native void onNativeSurfaceDestroyed();
    public static native void nativeSetNaturalOrientation(int orientation);
    public static native void onNativeRotationChanged(int rotation);
    public static native void onNativeInsetsChanged(int left, int right, int top, int bottom);

    // Keyboard
    public static native void onNativeScreenKeyboardShown();
    public static native void onNativeScreenKeyboardHidden();
    public static native void onNativeClipboardChanged();

    // Hints/environment
    public static native String nativeGetHint(String name);
    public static native boolean nativeGetHintBoolean(String name, boolean default_value);
    public static native void nativeSetenv(String name, String value);

    // Permissions/locale
    public static native void nativePermissionResult(int requestCode, boolean result);
    public static native void onNativeLocaleChanged();
    public static native void onNativeDarkModeChanged(boolean enabled);

    // File dialog
    public static native void onNativeFileDialog(int requestCode, String[] filelist, int filter);

    // Gestures
    public static native void onNativePinchStart();
    public static native void onNativePinchUpdate(float scale);
    public static native void onNativePinchEnd();

    // =========================================================================
    // Platform Detection Methods
    // These are actually used by QGC via JNI calls from SDLHelper
    // =========================================================================

    public static Activity getContext() {
        return SDL.getContext();
    }

    public static boolean isAndroidTV() {
        Activity context = SDL.getContext();
        if (context == null) return false;

        UiModeManager uiModeManager = (UiModeManager) context.getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager != null && uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            return true;
        }
        return context.getPackageManager().hasSystemFeature("android.software.leanback");
    }

    public static boolean isChromebook() {
        Activity context = SDL.getContext();
        if (context == null) return false;

        return context.getPackageManager().hasSystemFeature("org.chromium.arc.device_management") ||
               context.getPackageManager().hasSystemFeature("org.chromium.arc");
    }

    public static boolean isDeXMode() {
        Activity context = SDL.getContext();
        if (context == null) return false;

        Configuration config = context.getResources().getConfiguration();

        // Samsung DeX: check semDesktopModeEnabled field
        try {
            java.lang.reflect.Field field = Configuration.class.getField("semDesktopModeEnabled");
            if (field.getInt(config) == 1) return true;
        } catch (Exception e) { /* Not Samsung or no DeX */ }

        // Samsung DeX: check system property
        try {
            Class<?> sysProp = Class.forName("android.os.SystemProperties");
            java.lang.reflect.Method get = sysProp.getMethod("get", String.class, String.class);
            if ("1".equals(get.invoke(null, "persist.sys.force_desktop", ""))) return true;
        } catch (Exception e) { /* Property not accessible */ }

        return false;
    }

    public static boolean isTablet() {
        Activity context = SDL.getContext();
        if (context == null) return false;

        // Check screen size qualifier
        Configuration config = context.getResources().getConfiguration();
        int screenLayout = config.screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
        if (screenLayout >= Configuration.SCREENLAYOUT_SIZE_LARGE) return true;

        // Check physical screen size (>= 7 inches)
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        double widthInches = metrics.widthPixels / metrics.xdpi;
        double heightInches = metrics.heightPixels / metrics.ydpi;
        double diagonalInches = Math.sqrt(widthInches * widthInches + heightInches * heightInches);
        return diagonalInches >= 7.0;
    }

    // =========================================================================
    // Stub Methods Called by SDL Native Code
    // These must exist and return safe defaults. SDL looks them up at init time.
    // =========================================================================

    // Clipboard - Qt handles this
    public static String clipboardGetText() { return ""; }
    public static boolean clipboardHasText() { return false; }
    public static void clipboardSetText(String text) { }

    // Cursor - Qt handles this
    public static int createCustomCursor(int[] colors, int width, int height, int hotSpotX, int hotSpotY) { return 0; }
    public static void destroyCustomCursor(int cursorId) { }
    public static boolean setCustomCursor(int cursorId) { return false; }
    public static boolean setSystemCursor(int cursorId) { return false; }

    // Window/surface - Qt handles this
    public static Surface getNativeSurface() { return null; }
    public static View getContentView() { return null; }
    public static void setWindowStyle(boolean fullscreen) { }
    public static void setOrientation(int w, int h, boolean resizable, String hint) { }
    public static boolean setActivityTitle(String title) { return false; }
    public static void minimizeWindow() { }
    public static boolean shouldMinimizeOnFocusLoss() { return false; }

    // Input - Qt handles this
    public static void initTouch() { }
    public static boolean showTextInput(int x, int y, int w, int h, int inputType) { return false; }
    public static boolean setRelativeMouseEnabled(boolean enabled) { return false; }
    public static boolean supportsRelativeMouse() { return false; }
    public static void manualBackButton() { }

    // Misc - Qt handles these
    public static boolean getManifestEnvironmentVariables() { return false; }
    public static boolean openURL(String url) { return false; }
    public static void requestPermission(String permission, int requestCode) { }
    public static boolean showToast(String message, int duration, int gravity, int xOffset, int yOffset) { return false; }
    public static boolean sendMessage(int command, int param) { return false; }
    public static int openFileDescriptor(String path, String mode) { return -1; }
    public static boolean showFileDialog(String[] filters, boolean allowMultiple, boolean forWrite, int requestCode) { return false; }
    public static String getPreferredLocales() { return ""; }

    // =========================================================================
    // Methods Called by Other SDL Java Classes
    // =========================================================================

    /**
     * Called by SDLInputConnection to check if we're dispatching a key event.
     * Always returns false since Qt handles keyboard input.
     */
    public static boolean dispatchingKeyEvent() {
        return false;
    }

    /**
     * Called by SDLDummyEdit (if it existed) for keyboard input handling.
     * Returns false since Qt handles keyboard input.
     */
    public static boolean handleKeyEvent(View v, int keyCode, KeyEvent event, InputConnection ic) {
        return false;
    }
}
