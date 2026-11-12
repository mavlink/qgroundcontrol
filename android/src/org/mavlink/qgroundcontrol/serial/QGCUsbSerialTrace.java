package org.mavlink.qgroundcontrol.serial;

import android.util.Log;

/**
 * Logs what crosses each serial port to logcat, for debugging a link in the field.
 *
 * <p>Off unless enabled, without a rebuild or restart, by
 * {@code adb shell setprop log.tag.QGCSerial VERBOSE}.
 */
final class QGCUsbSerialTrace {
    private static final String TAG = "QGCSerial";
    private static final int MAX_BYTES_SHOWN = 32;
    private static final char[] HEX_DIGITS = "0123456789ABCDEF".toCharArray();

    private QGCUsbSerialTrace() {}

    static void transfer(final String direction, final int portId, final byte[] data, final int length) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "port " + portId + " " + direction + " " + length + ": " + hexOf(data, length));
        }
    }

    /** Returns the first {@code length} bytes of {@code data} in hex, cut short after {@code MAX_BYTES_SHOWN}. */
    static String hexOf(final byte[] data, final int length) {
        final int shown = Math.min(length, MAX_BYTES_SHOWN);
        final StringBuilder hex = new StringBuilder(3 * shown + 3);
        for (int i = 0; i < shown; i++) {
            hex.append(HEX_DIGITS[(data[i] >> 4) & 0xF]).append(HEX_DIGITS[data[i] & 0xF]).append(' ');
        }
        return (shown < length) ? hex.append("...").toString() : hex.toString();
    }

    static void event(final int portId, final String message) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "port " + portId + " " + message);
        }
    }
}
