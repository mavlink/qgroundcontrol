package org.mavlink.qgroundcontrol.serial;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Why a serial port can no longer transfer data, as reported to C++, where AndroidSerial::ResourceErrorReason
 * lists the same values.
 */
final class QGCUsbSerialResourceError {
    /**
     * Marks an int as one of the reasons below. Android lint, which the build runs, rejects any other value that
     * is passed where one is expected.
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({DEVICE_DETACHED, READ_FAILED, WRITE_FAILED})
    @interface Reason {}

    /** The device was unplugged. */
    static final int DEVICE_DETACHED = 1;
    /** A read from the device failed, and the device stayed attached. */
    static final int READ_FAILED = 2;
    /** A write to the device failed, and the device stayed attached. */
    static final int WRITE_FAILED = 3;

    private QGCUsbSerialResourceError() {}
}
