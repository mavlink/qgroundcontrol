package org.mavlink.qgroundcontrol.serial;

/** The calls into C++; {@code token} identifies the C++ port. Unit tests supply a recorder. */
interface QGCUsbSerialCallbacks {
    /**
     * The port has received the first {@code length} bytes of {@code data}. The callee must not keep the array:
     * it is the read thread's buffer, which the next read overwrites. C++ copies the bytes before it returns.
     *
     * <p>Returns whether C++ has room for more input. It has taken these bytes either way; after {@code false}
     * the read thread passes on no more until C++ starts reading again. The device still fills the read transfers
     * that are queued with it by then, and the thread passes that input on once C++ reads again.
     */
    boolean newData(long token, byte[] data, int length);

    /**
     * The port's write thread is done with this many of the bytes it was handed: it has written them, or a
     * discard of the port's output kept it from sending them.
     */
    void bytesWritten(long token, int count);

    /**
     * The port can no longer transfer data, for the given reason. C++ reports it as QSerialPort::ResourceError
     * with a text of its own for the reason, on which the port's owner closes the port. {@code detail} is what
     * the failure itself says, such as the message of its exception, for the log; it is empty if there is none,
     * never null.
     */
    void resourceError(long token, @QGCUsbSerialResourceError.Reason int reason, String detail);

    /** The ports of the attached devices, or which of them QGC may open, may have changed. */
    void devicesChanged();
}
