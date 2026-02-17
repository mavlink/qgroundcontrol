package org.libsdl.app;

/**
 * SDLAudioManager stub for QGroundControl
 *
 * SDL audio is disabled (SDL_AUDIO OFF), but SDL's JNI may still look for this class.
 * This provides minimal stubs to prevent ClassNotFoundException.
 */
class SDLAudioManager {

    static void initialize() { }
    static void setContext(android.content.Context context) { }
    static void release(android.content.Context context) { }
    static void registerAudioDeviceCallback() { }
    static void unregisterAudioDeviceCallback() { }
    static void audioSetThreadPriority(boolean recording, int device_id) { }

    // Native method declarations - required for JNI registration
    static native void nativeSetupJNI();
    static native void nativeRemoveAudioDevice(boolean recording, int deviceId);
    static native void nativeAddAudioDevice(boolean recording, String name, int deviceId);
}
