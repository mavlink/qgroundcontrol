package org.mavlink.qgroundcontrol.serial;

/** Constants shared across the serial subsystem — values that must agree with the JNI chunking contract. */
final class SerialConstants {

    private SerialConstants() {}

    /** Sentinel device-id returned on any open/setup failure. */
    static final int BAD_DEVICE_ID = 0;

    /** Default read buffer size handed to {@code SerialInputOutputManager}. */
    static final int READ_BUF_SIZE = 2048;

    // SerialInputOutputManager queue depth. usb-serial-for-android 3.10.0 release notes:
    // queuing multiple buffers prevents data loss when the JVM stalls (GC / JIT) between
    // kernel USB copies during permanent read() with timeout=0 at >115200 baud. 3 buffers
    // covers typical Android GC pauses (<30ms each) at MAVLink rates without growing
    // worst-case latency past one buffer-fill period.
    static final int READ_QUEUE_DEPTH = 3;

    /** Per-callback chunk size — Lifecycle must chunk to this; Manager guards. */
    static final int MAX_NATIVE_CALLBACK_DATA_BYTES = 16 * 1024;

    /** Direct ByteBuffer pool capacity — one buffer per open port's read thread is typical. */
    static final int DIRECT_BUFFER_POOL_CAP = 8;

    // Exception-kind ordinals delivered through nativeDeviceException. Must match
    // AndroidSerial::JavaExceptionKind in src/Android/AndroidSerial.h.
    static final int EXC_UNKNOWN     = 0;
    static final int EXC_RESOURCE    = 1;  // IOException at runtime — hot-unplug
    static final int EXC_PERMISSION  = 2;  // USB permission denied
    static final int EXC_OPEN_FAILED = 3;  // Open-path failure (driver / port / connection)

    // FTDI vendor control-transfer constants (AN232B-04). Used by VCP-mode FTDI in
    // UsbSerialLifecycle.openDriver to drop the chip latency timer from the 16ms
    // default; D2XX path goes through QGCFtdiSerialPort which uses FT_Device.setLatencyTimer.
    static final int FTDI_REQTYPE_OUT     = 0x40;  // vendor | host-to-device | device
    static final int FTDI_SIO_SET_LATENCY = 0x09;
    static final int FTDI_LATENCY_MS      = 1;     // 1ms minimum; default is 16ms
    static final int FTDI_CTRL_TIMEOUT_MS = 100;
}
