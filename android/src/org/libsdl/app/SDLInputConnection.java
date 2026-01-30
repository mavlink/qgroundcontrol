package org.libsdl.app;

/**
 * Minimal stub for SDL JNI registration.
 * Qt handles all input - this class only exists because SDL_android.c
 * registers native methods for it during JNI_OnLoad.
 */
class SDLInputConnection {
    public static native void nativeCommitText(String text, int newCursorPosition);
    public static native void nativeGenerateScancodeForUnichar(char c);
}
