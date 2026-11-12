package org.mavlink.qgroundcontrol.serial;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.io.IOException;
import java.util.Collection;
import java.util.function.IntFunction;

/**
 * USB serial access for the C++ AndroidSerial layer, which resolves the public static methods by name and
 * signature.
 *
 * <p>This class only connects Android and C++ to {@link QGCUsbSerialDevices}, which tracks the devices and
 * the ports opened on them, and to {@link QGCUsbSerialPort}, which operates one open port. It turns their
 * results and exceptions into the values C++ expects: a command returns a {@link QGCUsbSerialResult}, and a
 * query returns its answer, or a neutral one if it cannot tell. What a failure says of itself beyond its
 * result is kept for C++ to ask for ({@link #getLastErrorDetail}). The calls into Android's USB host API and
 * the receiver of its broadcasts are in {@link QGCUsbSerialAndroidHost}.
 *
 * <p>What a call does is described at the method of those two classes that it is passed to. A method here says
 * only what it adds to that, or how C++ hears of the outcome. AndroidSerial.h describes each call for its C++
 * callers.
 */
public final class QGCUsbSerialManager {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialManager";
    private static final byte[] NO_DATA = new byte[0];

    @FunctionalInterface
    private interface PortCall<T> {
        T on(QGCUsbSerialPort port) throws IOException;
    }

    @FunctionalInterface
    private interface PortCommand {
        void run(QGCUsbSerialPort port) throws IOException;
    }

    private static native void nativeDeviceResourceError(long token, int reason, String detail);
    private static native boolean nativeDeviceNewData(long token, byte[] data, int length);
    private static native void nativeDeviceBytesWritten(long token, int count);
    private static native void nativeDevicesChanged();

    private static final QGCUsbSerialCallbacks JNI_CALLBACKS = new QGCUsbSerialCallbacks() {
        @Override
        public boolean newData(final long token, final byte[] data, final int length) {
            return nativeDeviceNewData(token, data, length);
        }

        @Override
        public void bytesWritten(final long token, final int count) {
            nativeDeviceBytesWritten(token, count);
        }

        @Override
        public void resourceError(final long token, final int reason, final String detail) {
            nativeDeviceResourceError(token, reason, detail);
        }

        @Override
        public void devicesChanged() {
            nativeDevicesChanged();
        }
    };

    private static final QGCUsbSerialDevices sDevices = new QGCUsbSerialDevices(JNI_CALLBACKS);
    private static final Object sLifecycleLock = new Object();
    private static QGCUsbSerialAndroidHost sAndroidHost;

    private QGCUsbSerialManager() {}

    // ------------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------------

    public static void initialize(final Context context) {
        final QGCUsbSerialAndroidHost androidHost;
        synchronized (sLifecycleLock) {
            if (sAndroidHost != null) {
                return;
            }
            androidHost = QGCUsbSerialAndroidHost.start(context, QGCUsbSerialManager::onUsbEvent);
            if (androidHost == null) {
                return;
            }
            sAndroidHost = androidHost;
        }
        sDevices.start(androidHost, JNI_CALLBACKS);
    }

    static void initializeForTesting(final QGCUsbSerialHost usbHost, final QGCUsbSerialCallbacks callbacks) {
        sDevices.start(usbHost, callbacks);
    }

    public static void cleanup() {
        synchronized (sLifecycleLock) {
            if (sAndroidHost != null) {
                sAndroidHost.stop();
                sAndroidHost = null;
            }
        }
        sDevices.stop();
    }

    // ------------------------------------------------------------------------
    // USB events
    // ------------------------------------------------------------------------

    /** Called for each USB event, on the event thread of {@link QGCUsbSerialAndroidHost}. */
    static void onUsbEvent(final QGCUsbSerialEvent event, final UsbDevice device) {
        sDevices.onUsbEvent(event, device);
    }

    /**
     * Called when Android hands QGC a newly attached device, after the user accepted its prompt. That grants
     * the permission without a broadcast, so this is the only sign of it.
     */
    public static void onUsbDeviceGranted(final UsbDevice device) {
        sDevices.onDeviceGranted(device);
    }

    /**
     * Called on the UI thread when QGC's window gains or loses the focus. It has the focus while QGC is in front
     * with no dialog of the system on top, and only then does {@link #open} ask the user for a USB permission.
     */
    public static void onWindowFocusChanged(final boolean hasFocus) {
        synchronized (sLifecycleLock) {
            if (sAndroidHost != null) {
                sAndroidHost.setWindowFocused(hasFocus);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Failure detail (called from C++)
    // ------------------------------------------------------------------------

    /**
     * Returns what the failure of this thread's last call says of itself ({@link QGCUsbSerialLastError}). A write
     * that timed out counts as a failure here. Returns "" if the failure says nothing more, or if the call
     * succeeded. A call that can report a failure first forgets the detail of the one before, so C++ asks right
     * after the call that failed, on its thread.
     */
    public static String getLastErrorDetail() {
        return QGCUsbSerialLastError.get();
    }

    // ------------------------------------------------------------------------
    // Self-test (called from C++)
    // ------------------------------------------------------------------------

    /**
     * Calls each callback into C++ once, with the given arguments, so that C++ can compare what arrives with what
     * it sent. QGC's boot test runs it: until a device is plugged in, nothing else shows that the native methods
     * take their arguments as Java passes them. Whether C++ has room for more input is of no interest here.
     */
    @QGCUsbSerialResult.Code
    public static int echoToCallbacks(final long token, final byte[] data, final int length, final int written,
                                      @QGCUsbSerialResourceError.Reason final int reason, final String detail) {
        QGCUsbSerialLastError.clear();
        JNI_CALLBACKS.newData(token, data, length);
        JNI_CALLBACKS.bytesWritten(token, written);
        JNI_CALLBACKS.resourceError(token, reason, detail);
        JNI_CALLBACKS.devicesChanged();
        return QGCUsbSerialResult.OK;
    }

    // ------------------------------------------------------------------------
    // Port lookup
    // ------------------------------------------------------------------------

    private static QGCUsbSerialPort findOpenPort(final long handle, final String operation) {
        // Every call on an open port starts here, so that none passes off the detail of an earlier failure as its
        // own.
        QGCUsbSerialLastError.clear();
        final QGCUsbSerialPort port = sDevices.port(handle);
        if (port == null || !port.isOpen()) {
            QGCLogger.d(TAG, "Cannot " + operation + ": no port is open under this handle");
            return null;
        }
        return port;
    }

    /**
     * Makes a call on an open port and returns its answer. If the port is not open or the call fails, this logs
     * why, keeps what the failure says of itself, and returns what {@code onFailure} makes of the
     * {@link QGCUsbSerialResult} that tells how it failed.
     */
    private static <T> T onOpenPort(final long handle, final String operation, final PortCall<T> call,
                                    final IntFunction<T> onFailure) {
        final QGCUsbSerialPort port = findOpenPort(handle, operation);
        if (port == null) {
            return onFailure.apply(QGCUsbSerialResult.NOT_OPEN);
        }
        try {
            return call.on(port);
        } catch (final UnsupportedOperationException | IllegalArgumentException e) {
            QGCLogger.w(TAG, "Cannot " + operation + ": " + e.getMessage());
            QGCUsbSerialLastError.set(e);
            return onFailure.apply(QGCUsbSerialResult.UNSUPPORTED);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Failed to " + operation, e);
            QGCUsbSerialLastError.set(e);
            return onFailure.apply(QGCUsbSerialResult.IO_ERROR);
        } catch (final RuntimeException e) {
            // Closing the port clears the driver's connection without locking, under a call still in flight.
            if (port.isOpen()) {
                throw e;
            }
            QGCLogger.d(TAG, "Port " + port.id + " was closed during an attempt to " + operation);
            return onFailure.apply(QGCUsbSerialResult.NOT_OPEN);
        }
    }

    /** Asks an open port something. If that fails, returns the fallback instead. */
    private static <T> T query(final long handle, final String operation, final T fallback,
                               final PortCall<T> portQuery) {
        return onOpenPort(handle, operation, portQuery, failure -> fallback);
    }

    /** Runs a command on an open port and returns the {@link QGCUsbSerialResult} that says how it went. */
    @QGCUsbSerialResult.Code
    private static int command(final long handle, final String operation, final PortCommand portCommand) {
        return onOpenPort(handle, operation, port -> {
            portCommand.run(port);
            return QGCUsbSerialResult.OK;
        }, failure -> failure);
    }

    // A flow control or a control line crosses JNI as the number given here. AndroidSerial::FlowControl lists the
    // flow controls in this order, and kSignals in AndroidSerial.cc the lines. The numbers are spelled out rather
    // than taken from the library's enums, so that a constant it adds fails the build here and its order does
    // not matter.
    private static int flowControlCode(final UsbSerialPort.FlowControl flowControl) {
        return switch (flowControl) {
            case NONE -> 0;
            case RTS_CTS -> 1;
            case DTR_DSR -> 2;
            case XON_XOFF -> 3;
            case XON_XOFF_INLINE -> 4;
        };
    }

    private static int controlLineCode(final UsbSerialPort.ControlLine controlLine) {
        return switch (controlLine) {
            case RTS -> 0;
            case CTS -> 1;
            case DTR -> 2;
            case DSR -> 3;
            case CD -> 4;
            case RI -> 5;
        };
    }

    private static int[] flowControlCodes(final Collection<UsbSerialPort.FlowControl> flowControls) {
        return flowControls.stream().mapToInt(QGCUsbSerialManager::flowControlCode).toArray();
    }

    private static int[] controlLineCodes(final Collection<UsbSerialPort.ControlLine> controlLines) {
        return controlLines.stream().mapToInt(QGCUsbSerialManager::controlLineCode).toArray();
    }

    // ------------------------------------------------------------------------
    // Device enumeration (called from C++)
    // ------------------------------------------------------------------------

    public static QGCUsbSerialPortInfo[] availablePorts() {
        return sDevices.availablePorts();
    }

    public static boolean hasPermission(final String name) {
        return sDevices.hasPermission(name);
    }

    public static boolean requestPermission(final String name) {
        QGCUsbSerialLastError.clear();
        return sDevices.requestPermission(name);
    }

    /** Returns whether an attached device has a port with this name. */
    public static boolean isPortNameValid(final String name) {
        return sDevices.isPortNameValid(name);
    }

    public static long getPortHandle(final String name) {
        return sDevices.portHandle(name);
    }

    /** Returns the USB connection's file descriptor, or -1. */
    public static int getFileDescriptor(final long handle) {
        QGCUsbSerialLastError.clear();
        final QGCUsbSerialPort port = sDevices.port(handle);
        return (port != null) ? port.fileDescriptor() : -1;
    }

    // ------------------------------------------------------------------------
    // Open / close (called from C++)
    // ------------------------------------------------------------------------

    /**
     * Opens a port under {@code handle}, which C++ chooses, never 0. Every later call names the port by it, and
     * so do the callbacks, until the port is closed.
     *
     * <p>A port of a device that QGC may not open is not opened: the result is {@code PERMISSION_DENIED}, and
     * {@link #getLastErrorDetail} tells what has become of the permission. Such an open asks the user for it, who
     * answers later, if the device's {@link QGCUsbSerialPermissionPrompt} calls for that;
     * {@code nativeDevicesChanged} tells of a grant.
     */
    @QGCUsbSerialResult.Code
    public static int open(final String name, final long handle) {
        QGCUsbSerialLastError.clear();
        return sDevices.open(name, handle);
    }

    @QGCUsbSerialResult.Code
    public static int close(final long handle) {
        QGCUsbSerialLastError.clear();
        return sDevices.close(handle);
    }

    // ------------------------------------------------------------------------
    // Reading (called from C++)
    // ------------------------------------------------------------------------

    /** The input reaches C++ through {@code nativeDeviceNewData}. */
    @QGCUsbSerialResult.Code
    public static int startReading(final long handle) {
        final QGCUsbSerialPort port = findOpenPort(handle, "start reading");
        return (port != null) ? port.startReading() : QGCUsbSerialResult.NOT_OPEN;
    }

    @QGCUsbSerialResult.Code
    public static int stopReading(final long handle) {
        return command(handle, "stop reading", QGCUsbSerialPort::stopReading);
    }

    public static boolean isReading(final long handle) {
        final QGCUsbSerialPort port = sDevices.port(handle);
        return port != null && port.isReading();
    }

    // ------------------------------------------------------------------------
    // Read / write (called from C++)
    // ------------------------------------------------------------------------

    /** Returns no bytes also if the port is not open or the read fails. */
    public static byte[] read(final long handle, final int length, final int timeoutMs) {
        if (timeoutMs < 200) {
            QGCLogger.w(TAG, "Read with timeout less than recommended minimum of 200ms");
        }
        return query(handle, "read", NO_DATA, port -> port.read(length, timeoutMs));
    }

    /** Returns {@code NOT_OPEN} if the port is not open, otherwise what the port's write returns. */
    public static int writeWithProgress(final long handle, final byte[] data, final int length,
                                        final int timeoutMSec) {
        final QGCUsbSerialPort port = findOpenPort(handle, "write");
        return (port != null) ? port.writeWithProgress(data, length, timeoutMSec) : QGCUsbSerialResult.NOT_OPEN;
    }

    /**
     * C++ hears of the bytes through {@code nativeDeviceBytesWritten}, and of a write that failed through
     * {@code nativeDeviceResourceError}.
     */
    public static int writeAsync(final long handle, final byte[] data, final int length) {
        final QGCUsbSerialPort port = findOpenPort(handle, "write");
        return (port != null) ? port.writeAsync(data, length) : QGCUsbSerialResult.NOT_OPEN;
    }

    // ------------------------------------------------------------------------
    // Port configuration (called from C++)
    // ------------------------------------------------------------------------

    /**
     * Takes the constants of UsbSerialPort: 5 to 8 data bits; 1, 2 or, for one and a half, 3 stop bits; and
     * parity 0 none, 1 odd, 2 even, 3 mark, 4 space.
     */
    @QGCUsbSerialResult.Code
    public static int setParameters(final long handle, final int baudRate, final int dataBits, final int stopBits,
                                    final int parity) {
        return command(handle, "set parameters", port -> port.setParameters(baudRate, dataBits, stopBits, parity));
    }

    /** Returns the port's flow control as its {@link #flowControlCode}. */
    public static int getFlowControl(final long handle) {
        return query(handle, "get flow control", flowControlCode(UsbSerialPort.FlowControl.NONE),
                port -> flowControlCode(port.flowControl()));
    }

    @QGCUsbSerialResult.Code
    public static int setFlowControl(final long handle, final int flowControl) {
        QGCUsbSerialLastError.clear();
        for (final UsbSerialPort.FlowControl target : UsbSerialPort.FlowControl.values()) {
            if (flowControlCode(target) == flowControl) {
                return command(handle, "set flow control " + target, port -> port.setFlowControl(target));
            }
        }

        QGCLogger.w(TAG, "Invalid flow control code " + flowControl);
        return QGCUsbSerialResult.UNSUPPORTED;
    }

    /** Returns the {@link #flowControlCode} of each flow control the port supports. */
    public static int[] getSupportedFlowControl(final long handle) {
        return query(handle, "get supported flow control", new int[0],
                port -> flowControlCodes(port.supportedFlowControl()));
    }

    public static boolean getXon(final long handle) {
        return query(handle, "get XON", true, QGCUsbSerialPort::isXon);
    }

    @QGCUsbSerialResult.Code
    public static int setLatencyTimer(final long handle, final int milliseconds) {
        return command(handle, "set latency timer", port -> port.setLatencyTimer(milliseconds));
    }

    @QGCUsbSerialResult.Code
    public static int setBreak(final long handle, final boolean on) {
        return command(handle, "set break", port -> port.setBreak(on));
    }

    @QGCUsbSerialResult.Code
    public static int purgeBuffers(final long handle, final boolean input, final boolean output) {
        return command(handle, "purge buffers", port -> port.purgeBuffers(input, output));
    }

    // ------------------------------------------------------------------------
    // Control lines (called from C++)
    // ------------------------------------------------------------------------

    @QGCUsbSerialResult.Code
    public static int setDataTerminalReady(final long handle, final boolean on) {
        return command(handle, "set DTR", port -> port.setOutputLine(UsbSerialPort.ControlLine.DTR, on));
    }

    @QGCUsbSerialResult.Code
    public static int setRequestToSend(final long handle, final boolean on) {
        return command(handle, "set RTS", port -> port.setOutputLine(UsbSerialPort.ControlLine.RTS, on));
    }

    /** Returns the {@link #controlLineCode} of each active line. */
    public static int[] getControlLines(final long handle) {
        return query(handle, "get control lines", new int[0], port -> controlLineCodes(port.controlLines()));
    }

    /** Returns the {@link #controlLineCode} of each line the port has. */
    public static int[] getSupportedControlLines(final long handle) {
        return query(handle, "get supported control lines", new int[0],
                port -> controlLineCodes(port.supportedControlLines()));
    }
}
