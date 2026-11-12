package org.mavlink.qgroundcontrol.serial;

/**
 * Whether QGC asks the user to let it open one attached device, when a port of the device cannot be opened for
 * lack of that permission.
 *
 * <p>C++ opens a port that it recognises without being told to, and tries again every few seconds. Such an open
 * asks the user, who answers later, if all of this holds:
 * <ul>
 *   <li>QGC has not asked since the device was attached, or Android has closed the dialog of its request without
 *       an answer, as it does when the user accepts its prompt for another device on top of that dialog. The
 *       user so gets one dialog at a time, and a denial stands until the device is replugged or
 *       {@link QGCUsbSerialDevices#requestPermission} asks again.
 *   <li>The device has been attached for long enough that Android's own prompt for it is up.
 *   <li>The user would get the request as the only dialog ({@link QGCUsbSerialHost#canAskUser}), which is not
 *       so while that prompt is up.
 * </ul>
 *
 * <p>{@link QGCUsbSerialDevices} keeps one for each device that it tracks, and uses it under its lock.
 */
final class QGCUsbSerialPermissionPrompt {
    // Android's own prompt for a device that is attached takes about this long to appear. Until then QGC does not
    // ask for the device of its own accord, or the user would get its dialog as a second one, under that prompt.
    static final long ATTACH_GRACE_MS = 2000;
    // The dialog of a request takes a moment to cover QGC's window. For this long a request that has no answer
    // counts as waiting for one, though the user could be asked.
    static final long REQUEST_GRACE_MS = 2000;

    /** What has become of the request to let QGC open the device, since the device was attached. */
    private enum Request {
        NOT_ASKED,
        /** The user has been asked and has not answered. */
        PENDING,
        DENIED
    }

    /**
     * What an open that is refused for lack of permission does about it: whether it asks the user, whether it is
     * the first to be refused since the device was attached, and what has become of the permission.
     */
    record Refusal(boolean asksUser, boolean isFirst, String detail) {}

    // By the clock of the devices. A device found at startup was attached earlier, which does not matter: QGC
    // asks for it there, and so never again of its own accord.
    private final long _attachedAtMs;
    private long _askedAtMs;
    private Request _request = Request.NOT_ASKED;
    private boolean _openRefused;

    QGCUsbSerialPermissionPrompt(final long attachedAtMs) {
        _attachedAtMs = attachedAtMs;
    }

    /** Notes that the user has been asked at {@code nowMs}, so that no open asks as well. */
    void asked(final long nowMs) {
        _request = Request.PENDING;
        _askedAtMs = nowMs;
    }

    /** Notes that the user has denied the request. */
    void denied() {
        _request = Request.DENIED;
    }

    /** Decides what an open at {@code nowMs} does about the permission. A request that it calls for counts as made. */
    Refusal refuseOpen(final long nowMs, final boolean canAskUser) {
        final boolean justAttached = nowMs - _attachedAtMs < ATTACH_GRACE_MS;
        // The dialog of a request that waits for its answer keeps the user from being asked. Where they can be,
        // Android has closed it without one.
        final boolean answerOverdue = _request == Request.PENDING && nowMs - _askedAtMs >= REQUEST_GRACE_MS;
        final boolean asksUser = canAskUser && ((_request == Request.NOT_ASKED && !justAttached) || answerOverdue);
        final String detail;
        if (asksUser) {
            asked(nowMs);
            detail = "The user has been asked to allow it";
        } else if (_request == Request.PENDING) {
            detail = "The user has not answered the request yet";
        } else if (_request == Request.DENIED) {
            detail = "The user denied it; replug the device or connect to it by hand to be asked again";
        } else if (justAttached) {
            detail = "The device was just attached, and Android asks the user first";
        } else {
            detail = "The user is not asked while another window has the focus";
        }
        final boolean isFirst = !_openRefused;
        _openRefused = true;
        return new Refusal(asksUser, isFirst, detail);
    }
}
