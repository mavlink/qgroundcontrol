package org.mavlink.qgroundcontrol;

import android.util.Log;

import java.util.function.Supplier;

/**
 * A centralized logging utility that manages log messages across the application.
 * It controls log levels and formats based on build configurations.
 *
 * <p>Each log call is mirrored to {@link QGCNativeLogSink} so that Java log
 * output appears in the Qt logging system (and therefore in QGC's log viewer).
 * If the native library is not yet loaded, the mirror call is silently skipped
 * (see {@link QGCNativeLogSink#log}).
 *
 * <p>The hot-path {@code d}/{@code v} (debug/verbose) methods carry
 * {@link Supplier Supplier&lt;String&gt;} overloads that defer message
 * construction until after the level gate:
 * <pre>
 *     QGCLogger.d(TAG, () -&gt; "Large payload (" + data.length + " bytes)");
 * </pre>
 * The lambda is only invoked when the level is enabled, eliminating the
 * per-call allocation on calls the short-circuit gate would have dropped.
 * {@code i}/{@code w}/{@code e} always log, so they take plain strings only.
 */
public class QGCLogger {
    // Determine if the build is a debug build
    private static final boolean DEBUG = BuildConfig.DEBUG;

    /** True if debug-level logging is enabled.  Callers on hot paths can
     *  guard expensive diagnostics themselves: {@code if (QGCLogger.isDebugEnabled()) ...}. */
    public static boolean isDebugEnabled() {
        return DEBUG;
    }

    /**
     * Logs a debug message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void d(String tag, String message) {
        if (DEBUG) {
            Log.d(tag, message);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, message);
        }
    }

    /** Debug log that defers message construction via a {@link Supplier}. */
    public static void d(String tag, Supplier<String> message) {
        if (DEBUG) {
            final String msg = message.get();
            Log.d(tag, msg);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, msg);
        }
    }

    /** Verbose log gated by per-tag {@code adb shell setprop log.tag.<TAG> VERBOSE} (≤23 chars). */
    public static void v(String tag, String message) {
        if (Log.isLoggable(tag, Log.VERBOSE)) {
            Log.v(tag, message);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, message);
        }
    }

    /** Verbose log enabled by either the class tag or a package-wide parent tag. */
    public static void v(String tag, String parentTag, String message) {
        if (Log.isLoggable(tag, Log.VERBOSE) || Log.isLoggable(parentTag, Log.VERBOSE)) {
            Log.v(tag, message);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, message);
        }
    }

    public static void v(String tag, Supplier<String> message) {
        if (Log.isLoggable(tag, Log.VERBOSE)) {
            final String msg = message.get();
            Log.v(tag, msg);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, msg);
        }
    }

    public static void v(String tag, String parentTag, Supplier<String> message) {
        if (Log.isLoggable(tag, Log.VERBOSE) || Log.isLoggable(parentTag, Log.VERBOSE)) {
            final String msg = message.get();
            Log.v(tag, msg);
            QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_DEBUG, tag, msg);
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
        QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_INFO, tag, message);
    }

    /**
     * Logs a warning message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void w(String tag, String message) {
        Log.w(tag, message);
        QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_WARNING, tag, message);
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
        QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_WARNING, tag,
                message + ": " + throwable.getMessage());
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
        QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_ERROR, tag,
                message + ": " + throwable.getMessage());
    }

    /**
     * Logs an error message without a throwable.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void e(String tag, String message) {
        Log.e(tag, message);
        QGCNativeLogSink.log(QGCNativeLogSink.LEVEL_ERROR, tag, message);
    }
}
