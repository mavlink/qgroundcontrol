package org.mavlink.qgroundcontrol.serial;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * How an operation on a serial port went, as reported to C++, where AndroidSerial::Result lists the same
 * values. A method that returns a count returns one of the negative values instead when it fails.
 */
final class QGCUsbSerialResult {
    /**
     * Marks an int as one of the results below. Android lint, which the build runs, rejects any other value that
     * a method so marked would return.
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({OK, FAILED, IO_ERROR, UNSUPPORTED, NOT_OPEN, DEVICE_NOT_FOUND, PERMISSION_DENIED, ALREADY_OPEN,
             PERMISSION_REQUESTED, PERMISSION_PENDING, PERMISSION_AWAITS_ANDROID, PERMISSION_AWAITS_FOCUS,
             AMBIGUOUS_NAME})
    @interface Code {}

    static final int OK = 0;
    /** It failed for a reason not listed here. */
    static final int FAILED = -1;
    /** The device did not carry it out, and may be gone. */
    static final int IO_ERROR = -2;
    /** The port or its driver cannot do this. */
    static final int UNSUPPORTED = -3;
    /** No port is open under this handle. */
    static final int NOT_OPEN = -4;
    /** Opening: there is no such device. */
    static final int DEVICE_NOT_FOUND = -5;
    /** Opening: QGC may not use the device, and the user denied the request to allow it. */
    static final int PERMISSION_DENIED = -6;
    /** Opening: the port is already open. */
    static final int ALREADY_OPEN = -7;
    /** Opening: QGC may not use the device, and this open has asked the user to allow it. */
    static final int PERMISSION_REQUESTED = -8;
    /** Opening: QGC may not use the device, and the user was asked before and has not answered yet. */
    static final int PERMISSION_PENDING = -9;
    /** Opening: QGC may not use the device, which was just attached, and Android asks the user first. */
    static final int PERMISSION_AWAITS_ANDROID = -10;
    /** Opening: QGC may not use the device, and the user is not asked while another window has the focus. */
    static final int PERMISSION_AWAITS_FOCUS = -11;
    /** Opening: several attached devices have this stable name, which then opens none of them. */
    static final int AMBIGUOUS_NAME = -12;

    private QGCUsbSerialResult() {}
}
