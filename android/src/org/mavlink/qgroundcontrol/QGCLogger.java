package org.mavlink.qgroundcontrol;

import android.util.Log;

/**
 * A centralized logging utility that manages log messages across the application.
 * It controls log levels and formats based on build configurations.
 */
public class QGCLogger {
    // Determine if the build is a debug build
    private static final boolean DEBUG = BuildConfig.DEBUG;

    /**
     * Logs a debug message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void d(String tag, String message) {
        if (DEBUG) {
            Log.d(tag, message);
        }
    }

    /**
     * Logs an informational message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void i(String tag, String message) {
        Log.i(tag, message);
    }

    /**
     * Logs a warning message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void w(String tag, String message) {
        Log.w(tag, message);
    }

    /**
     * Logs an error message along with a throwable.
     *
     * @param tag       The source of the log message.
     * @param message   The message to log.
     * @param throwable The throwable to log.
     */
    public static void e(String tag, String message, Throwable throwable) {
        Log.e(tag, message, throwable);
    }

    /**
     * Logs an error message without a throwable.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void e(String tag, String message) {
        Log.e(tag, message);
    }
}
