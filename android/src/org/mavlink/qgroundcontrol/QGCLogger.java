package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.util.Log;

/**
 * A centralized logging utility that manages log messages across the application.
 * It controls log levels and formats based on build configurations.
 */
public class QGCLogger {
    // Whether this is a debuggable build. Determined at runtime instead of via
    // BuildConfig, since BuildConfig is generated in the (customizable) app
    // namespace package rather than this fixed source package.
    private static volatile boolean sDebug = false;

    /**
     * Initializes the logger. Should be called once, typically from QGCActivity.onCreate().
     *
     * @param context Any application context.
     */
    public static void initialize(Context context) {
        sDebug = (context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    }

    /**
     * Logs a debug message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void d(String tag, String message) {
        if (sDebug) {
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
     * Logs a warning message along with a throwable.
     *
     * @param tag       The source of the log message.
     * @param message   The message to log.
     * @param throwable The throwable to log.
     */
    public static void w(String tag, String message, Throwable throwable) {
        Log.w(tag, message, throwable);
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
