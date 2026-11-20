package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.util.Log;

import java.util.concurrent.atomic.AtomicReference;

/**
 * A centralized logging utility that manages log messages across the application.
 * It controls log levels and formats based on build configurations.
 *
 * <p>Informational messages, warnings and errors also go to QGC's own log, which the user can view and export in
 * the app, while the activity has set a {@link Sink} for them. Debug messages go to Android's log only.
 */
public class QGCLogger {
    /** QGC's own log, as far as this class needs it. Its methods are called on whichever thread logs. */
    interface Sink {
        /** Takes a message that QGC's log shows only while the user has turned its category on. */
        void debug(String message);

        /** Takes a message that QGC's log always shows. */
        void warning(String message);
    }

    // Whether this is a debuggable build. Determined at runtime instead of via
    // BuildConfig, since BuildConfig is generated in the (customizable) app
    // namespace package rather than this fixed source package.
    private static volatile boolean sDebug = false;
    // Null while there is no activity, and once the sink has failed.
    private static final AtomicReference<Sink> sSink = new AtomicReference<>();

    /**
     * Initializes the logger. Should be called once, typically from QGCActivity.onCreate().
     *
     * @param context Any application context.
     */
    public static void initialize(Context context) {
        sDebug = (context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    }

    /**
     * Sets where informational messages, warnings and errors go besides Android's log; with null, nowhere else.
     * For QGCActivity, whose native methods reach QGC's own log, and only while those can be called.
     */
    static void setSink(final Sink sink) {
        sSink.set(sink);
    }

    /**
     * Hands a message to the sink, with its tag in front. A sink that fails gets no further message: without the
     * native library, as in a unit test, the activity's would fail for every message alike.
     */
    private static void forward(final String tag, final String message, final boolean isWarning) {
        final Sink sink = sSink.get();
        if (sink == null) {
            return;
        }
        final String taggedMessage = tag + ": " + message;
        try {
            if (isWarning) {
                sink.warning(taggedMessage);
            } else {
                sink.debug(taggedMessage);
            }
        } catch (final RuntimeException | LinkageError e) {
            sSink.compareAndSet(sink, null);
        }
    }

    /** A message with what its throwable says of itself: the throwable's message or, if it has none, its name. */
    private static String withCause(final String message, final Throwable throwable) {
        if (throwable == null) {
            return message;
        }
        final String cause = throwable.getMessage();
        return message + ": " + ((cause != null) ? cause : throwable.getClass().getSimpleName());
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
        forward(tag, message, false);
    }

    /**
     * Logs a warning message.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void w(String tag, String message) {
        Log.w(tag, message);
        forward(tag, message, true);
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
        forward(tag, withCause(message, throwable), true);
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
        forward(tag, withCause(message, throwable), true);
    }

    /**
     * Logs an error message without a throwable.
     *
     * @param tag     The source of the log message.
     * @param message The message to log.
     */
    public static void e(String tag, String message) {
        Log.e(tag, message);
        forward(tag, message, true);
    }
}
