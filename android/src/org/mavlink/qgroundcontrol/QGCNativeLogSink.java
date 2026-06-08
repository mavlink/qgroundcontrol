package org.mavlink.qgroundcontrol;

/**
 * Bridges Java log output into the QGC native (Qt) logging system.
 *
 * <p>Each log call is forwarded via JNI to {@code nativeLog}, which maps the
 * level ordinal to the appropriate {@code qCDebug/qCInfo/qCWarning/qCCritical}
 * category on the C++ side.
 *
 * <p>JNI availability is tested lazily on the first call.  If the native
 * library is not yet loaded (e.g. during early unit-test bootstrap), the
 * {@link UnsatisfiedLinkError} is caught once and subsequent calls are
 * silently skipped.
 */
public final class QGCNativeLogSink {

    /** Log-level ordinals — must match the C++ side mapping in AndroidLogSink.cc. */
    public static final int LEVEL_DEBUG   = 0;
    public static final int LEVEL_INFO    = 1;
    public static final int LEVEL_WARNING = 2;
    public static final int LEVEL_ERROR   = 3;

    private static final java.util.concurrent.atomic.AtomicBoolean sJniAvailable = new java.util.concurrent.atomic.AtomicBoolean(true);

    private QGCNativeLogSink() { /* utility class */ }

    /**
     * Forwards a log entry to native.
     *
     * @param level   one of {@link #LEVEL_DEBUG}, {@link #LEVEL_INFO},
     *                {@link #LEVEL_WARNING}, {@link #LEVEL_ERROR}.
     * @param tag     log tag (typically the calling class name).
     * @param message log message.
     */
    public static void log(final int level, final String tag, final String message) {
        if (!sJniAvailable.get()) {
            return;
        }
        try {
            nativeLog(level, tag, message);
        } catch (final UnsatisfiedLinkError e) {
            sJniAvailable.compareAndSet(true, false);
        }
    }

    /**
     * Native entry point registered by {@code AndroidLogSink.cc}.
     *
     * @param level   log level ordinal (see {@link #LEVEL_DEBUG} etc.).
     * @param tag     log tag.
     * @param message log message.
     */
    public static native void nativeLog(int level, String tag, String message);
}
