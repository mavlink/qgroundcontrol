package org.mavlink.qgroundcontrol.serial;

/**
 * What the failure of each thread's last call says of itself beyond its {@link QGCUsbSerialResult}: the message
 * of the exception behind it, or a short text where no exception tells the reason and the result leaves it open.
 *
 * <p>Kept per thread, as errno is: C++ asks on the thread of the call that failed, and a failed open leaves no
 * port that could keep it. A port is also closed on threads that C++ never calls on, as when its device detaches;
 * what is kept there is never asked for.
 */
final class QGCUsbSerialLastError {
    private static final ThreadLocal<String> sDetail = new ThreadLocal<>();

    private QGCUsbSerialLastError() {}

    /** Forgets the detail. A call that can report a failure starts with this, so that it passes off none as its own. */
    static void clear() {
        sDetail.remove();
    }

    /** Keeps the message of the exception behind a failure. One without a message leaves the detail empty. */
    static void set(final Exception e) {
        set(e.getMessage());
    }

    /** Keeps a short text for a failure that no exception tells the reason of. */
    static void set(final String detail) {
        sDetail.set(detail);
    }

    /** Returns the detail, or "" if this thread's last call left none. */
    static String get() {
        final String detail = sDetail.get();
        return (detail != null) ? detail : "";
    }
}
