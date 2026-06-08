package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.EXC_OPEN_FAILED;
import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.EXC_RESOURCE;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;

import org.qtproject.qt.android.UsedFromNativeCode;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.IOException;
import java.nio.ByteBuffer;

/** One Java object per open Android USB serial port; hosts a write loop and a read loop over the shared lifecycle monitor. */
public class QGCSerialPort {

    private static final String TAG = QGCSerialPort.class.getSimpleName();

    // REGISTERED gates configure(); CONFIGURED means reported to the sink (so onPortClosed only fires for those); CLOSING/CLOSED gate close() re-entry.
    enum LifecycleState { REGISTERED, CONFIGURED, CLOSING, CLOSED }

    enum CloseReason {
        USER(false),
        // Both DETACHED (broadcast) and DEVICE_ERROR (onRunError IOException) mean the device is gone; the C++ side must learn either way (#1A).
        DETACHED(true),
        STALE_DRIVER(true),
        DEVICE_ERROR(true),
        SHUTDOWN(false),
        OPEN_ROLLBACK(false);

        final boolean emitDisconnected;

        CloseReason(final boolean emitDisconnected) {
            this.emitDisconnected = emitDisconnected;
        }
    }

    interface PortLifecycleSink {
        void onPortConfigured(QGCSerialPort port);
        void onPortClosed(QGCSerialPort port);
        void onPortDeviceError(QGCSerialPort port);
    }

    /** Seam over the four JNI entry points so write/read/lifecycle logic is testable without the native layer (tests inject a fake via setNativeBridgeForTest). */
    interface NativeBridge {
        void deviceHasDisconnected(long handle);
        void deviceException(long handle, int kind, String message);
        void deviceNewData(long handle, ByteBuffer data, int length);
        void deviceBytesWritten(long handle, int n);
    }

    private final QGCUsbSerialManager.PortAddress address;
    private final UsbSerialPort port;
    /** Same object as {@link #port} when it is the D2XX-backed FTDI port (the only one with QGC hooks), else null; resolved once. */
    private final QGCFtdiSerialPort extendedPort;
    /** Per-driver quirks as one immutable object so write/read paths read capabilities without touching the driver's runtime type. */
    private final DriverStrategy.Caps caps;
    static final int WRITE_QUEUE_CAPACITY = SerialWriteLoop.WRITE_QUEUE_CAPACITY;
    // Declared before the loops: each captures host.lock() in its constructor, so lifecycleLock must already be initialized.
    private final Object lifecycleLock = new Object();
    private final HostImpl host = new HostImpl();
    // Synchronous writer (not ioManager.writeAsync): p.write() lets us ack each sub-write via nativeDeviceBytesWritten for C++ backpressure.
    private final SerialWriteLoop writeLoop = new SerialWriteLoop(host);
    private final SerialReadLoop readLoop = new SerialReadLoop(host);
    private final PortLifecycleSink portLifecycleSink;
    private final Runnable unregisterCallback;
    /** Zeroed by close() so an in-flight emit/exception after the C++ handle drain is a no-op (Java-side belt-and-suspenders to the C++ QReadWriteLock). */
    private volatile long nativeHandle;
    private final UsbManager usbManager;
    private final UsbSerialDriver driver;
    /** Owned host USB connection. Null for D2XX (which owns its own internally); set otherwise. Guarded by lifecycleLock. */
    private UsbDeviceConnection connection;
    private int currentBaudRate;
    /** Forward-only lifecycle state. All mutation flows through transitionLocked under lifecycleLock. */
    private LifecycleState lifecycleState = LifecycleState.REGISTERED;
    private boolean disconnectEmitted;
    private volatile boolean listenerMuted;

    QGCSerialPort(final QGCUsbSerialManager.PortAddress address,
            final UsbManager usbManager,
            final UsbSerialDriver driver,
            final UsbSerialPort port,
            final long nativeHandle,
            final PortLifecycleSink portLifecycleSink,
            final Runnable unregisterCallback) {
        this.address = address;
        this.usbManager = usbManager;
        this.driver = driver;
        this.port = port;
        this.extendedPort = (port instanceof QGCFtdiSerialPort ext) ? ext : null;
        this.caps = DriverStrategy.capsFor(driver);
        this.nativeHandle = nativeHandle;
        this.portLifecycleSink = portLifecycleSink;
        this.unregisterCallback = unregisterCallback;
    }

    // JNI bridge — bound by name from C++ native registration on this per-port class.
    private native void nativeDeviceHasDisconnected(final long nativeHandle);
    private native void nativeDeviceException(final long nativeHandle, final int kind, final String message);
    /** JNI bridge — direct ByteBuffer for GetDirectBufferAddress; native code must consume it synchronously (recycled after return). */
    private native void nativeDeviceNewData(final long nativeHandle,
            final java.nio.ByteBuffer data, final int length);
    private native void nativeDeviceBytesWritten(final long nativeHandle, final int n);

    private volatile NativeBridge nativeBridge = new RealNativeBridge();

    private final class RealNativeBridge implements NativeBridge {
        @Override public void deviceHasDisconnected(final long handle) { nativeDeviceHasDisconnected(handle); }
        @Override public void deviceException(final long handle, final int kind, final String message) { nativeDeviceException(handle, kind, message); }
        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { nativeDeviceNewData(handle, data, length); }
        @Override public void deviceBytesWritten(final long handle, final int n) { nativeDeviceBytesWritten(handle, n); }
    }

    void setNativeBridgeForTest(final NativeBridge bridge) {
        this.nativeBridge = bridge;
    }

    void forceConfiguredForTest(final int baudRate) {
        synchronized (lifecycleLock) {
            currentBaudRate = baudRate;
            transitionLocked(LifecycleState.CONFIGURED);
            writeLoop.startLocked(String.valueOf(address));
        }
    }

    boolean awaitWriteLoopDrainedForTest(final long timeoutMs) throws InterruptedException {
        return writeLoop.awaitDrainedForTest(timeoutMs);
    }

    SerialReadLoop readLoopForTest() {
        return readLoop;
    }

    void muteListenerForTest() {
        synchronized (lifecycleLock) {
            host.muteListenerLocked();
        }
    }

    QGCUsbSerialManager.PortAddress address() { return address; }
    long nativeHandle() { return nativeHandle; }

    String stateForLogging() {
        synchronized (lifecycleLock) {
            return lifecycleState.name();
        }
    }

    public boolean configure(final QGCUsbSerialManager.SerialParameters params,
            final int flowControl, final boolean assertDtr) {
        synchronized (lifecycleLock) {
            if (lifecycleState != LifecycleState.REGISTERED) {
                QGCLogger.w(TAG, "configure requested in state " + lifecycleState + " for " + address);
                return false;
            }
            if (!openLocked(port, address)) {
                closeLocked(CloseReason.OPEN_ROLLBACK);
                return false;
            }
            if (!readLoop.createIoManagerLocked()) {
                fireException(EXC_OPEN_FAILED, "Failed to start I/O for device: " + address);
                closeLocked(CloseReason.OPEN_ROLLBACK);
                return false;
            }
            if (!setSerialParametersLocked(params)
                    || !setFlowControlLocked(flowControl)
                    || (assertDtr && !setControlLineLocked(UsbSerialPort.ControlLine.DTR, true))) {
                QGCLogger.e(TAG, "Failed to apply post-open config for " + address + "; rolling back");
                closeLocked(CloseReason.OPEN_ROLLBACK);
                return false;
            }
            notifyConfiguredLocked();
            return true;
        }
    }

    /** Opens {@code targetPort} (per-driver quirks; the D2XX T1/T2 claim ordering must stay one contiguous critical section). Caller holds lifecycleLock; on failure fireException has fired and any connection is released. */
    private boolean openLocked(final UsbSerialPort targetPort, final QGCUsbSerialManager.PortAddress addr) {
        if (usbManager == null || driver == null || targetPort == null) {
            fireException(EXC_OPEN_FAILED, "No serial port available for device: " + addr);
            return false;
        }
        if (targetPort.isOpen()) {
            return true;
        }

        final UsbDevice device = driver.getDevice();
        final UsbDeviceConnection openedConnection = usbManager.openDevice(device);
        if (openedConnection == null) {
            // Detach-during-open gap (#1D): device gone between registration and open routes through EXC_RESOURCE so C++ surfaces ResourceUnavailable, not a generic failure.
            final boolean stillPresent = usbManager.getDeviceList().containsKey(device.getDeviceName());
            final int kind = stillPresent ? EXC_OPEN_FAILED : EXC_RESOURCE;
            fireException(kind, "No USB device connection for device: " + device.getDeviceName());
            return false;
        }

        // D2XX race fix (#14146): FT_Device.openDevice re-calls UsbManager.openDevice() + claimInterface(force=true),
        // revoking/leaking T1's claim (and returning null on some Android 12+ OEMs); close the T1 probe before D2XX opens T2.
        if (caps.usesD2xx()) {
            openedConnection.close();
            try {
                targetPort.open(null);
            } catch (final IOException ex) {
                QGCLogger.e(TAG, "Error opening driver for device " + device.getDeviceName(), ex);
                fireException(EXC_OPEN_FAILED, "Error opening driver: " + ex.getMessage());
                return false;
            }
            // D2XX owns the connection internally — no Java-side handle to track.
            connection = null;
            return true;
        }

        try {
            targetPort.open(openedConnection);
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error opening driver for device " + device.getDeviceName(), ex);
            fireException(EXC_OPEN_FAILED, "Error opening driver: " + ex.getMessage());
            openedConnection.close();
            return false;
        }

        if (caps.needsHostFtdiLatencyTimer()) {
            DriverStrategy.applyHostFtdiLatencyTimer(openedConnection, addr.portIndex());
        }

        connection = openedConnection;
        return true;
    }

    /** Legal lifecycle transitions (forward only): REGISTERED→CONFIGURED, REGISTERED/CONFIGURED→CLOSING, CLOSING→CLOSED. */
    static boolean isLifecycleTransitionAllowed(final LifecycleState from, final LifecycleState to) {
        switch (from) {
            case REGISTERED: return to == LifecycleState.CONFIGURED || to == LifecycleState.CLOSING;
            case CONFIGURED: return to == LifecycleState.CLOSING;
            case CLOSING:    return to == LifecycleState.CLOSED;
            default:         return false;
        }
    }

    /** All lifecycleState mutation flows through here. Caller holds lifecycleLock. */
    private void transitionLocked(final LifecycleState to) {
        final LifecycleState from = lifecycleState;
        if (from == to) {
            return;
        }
        if (!isLifecycleTransitionAllowed(from, to)) {
            QGCLogger.e(TAG, "Illegal lifecycle transition " + from + " -> " + to + " for " + address);
            return;
        }
        lifecycleState = to;
    }

    /** Transitions REGISTERED → CONFIGURED and fires the sink. Idempotent. */
    private void notifyConfiguredLocked() {
        if (lifecycleState == LifecycleState.REGISTERED) {
            transitionLocked(LifecycleState.CONFIGURED);
            if (portLifecycleSink != null) {
                portLifecycleSink.onPortConfigured(this);
            }
        }
    }

    /** {@code wasConfigured} captures pre-CLOSING state so onPortClosed fires only for ports reported configured upstream. */
    private void notifyClosedLocked(final boolean wasConfigured) {
        if (wasConfigured && portLifecycleSink != null) {
            portLifecycleSink.onPortClosed(this);
        }
    }

    @UsedFromNativeCode
    public boolean close() {
        return close(CloseReason.USER);
    }

    public boolean close(final CloseReason reason) {
        // Preempt any in-flight blocking write before taking lifecycleLock, else close() sits behind a wedged D2XX write (multi-second); cancelPendingWrites interrupts the parked writer, no-op on non-D2XX.
        cancelPendingWritesUnlocked();
        writeLoop.stopUnlocked(String.valueOf(address));
        synchronized (lifecycleLock) {
            return closeLocked(reason);
        }
    }

    private void cancelPendingWritesUnlocked() {
        if (extendedPort != null) {
            extendedPort.cancelPendingWrites();
        }
    }

    private boolean closeLocked(final CloseReason reason) {
        if (lifecycleState == LifecycleState.CLOSED) {
            return true;
        }
        if (lifecycleState == LifecycleState.CLOSING) {
            return true;
        }

        final boolean wasConfigured = (lifecycleState == LifecycleState.CONFIGURED);
        transitionLocked(LifecycleState.CLOSING);

        // stopIoManagerLocked() mutes the listener as its first action, so no separate mute is needed here.
        readLoop.stopIoManagerLocked(address);
        final boolean ok = closePortLocked();
        closeConnectionLocked();
        unregisterLocked();
        transitionLocked(LifecycleState.CLOSED);
        notifyClosedLocked(wasConfigured);
        maybeEmitDisconnectLocked(reason);
        // After the disconnect emit, no further JNI traffic is valid for this port.
        nativeHandle = 0L;
        return ok;
    }

    boolean closePortLocked() {
        if (port == null || !port.isOpen()) {
            return true;
        }
        try {
            port.close();
            QGCLogger.d(TAG, "Device " + address + " closed successfully.");
            return true;
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error closing driver:", ex);
            return false;
        }
    }

    /** Releases the owned host connection. Caller holds lifecycleLock. */
    void closeConnectionLocked() {
        if (connection != null) {
            try { connection.close(); }
            catch (final Throwable t) { QGCLogger.w(TAG, "Error closing UsbDeviceConnection: " + t.getMessage()); }
            connection = null;
        }
    }

    void unregisterLocked() {
        if (unregisterCallback != null) {
            unregisterCallback.run();
        }
    }

    @UsedFromNativeCode
    public boolean startIoManager() {
        synchronized (lifecycleLock) {
            // Only a CONFIGURED port has a live IO manager; start() on a STOPPING SerialInputOutputManager throws IllegalStateException (hoho.android.usbserial), so fail fast on lifecycle.
            if (lifecycleState != LifecycleState.CONFIGURED) {
                QGCLogger.w(TAG, "startIoManager rejected in state " + lifecycleState + " for " + address);
                return false;
            }
            if (!readLoop.startLocked(address)) {
                return false;
            }
            if (!writeLoop.startLocked(String.valueOf(address))) {
                readLoop.stopIoManagerLocked(address);
                return false;
            }
            return true;
        }
    }

    @UsedFromNativeCode
    public boolean stopIoManager() {
        synchronized (lifecycleLock) {
            return readLoop.stopIoManagerLocked(address);
        }
    }

    @UsedFromNativeCode
    public boolean setSerialParameters(final QGCUsbSerialManager.SerialParameters params) {
        synchronized (lifecycleLock) {
            return setSerialParametersLocked(params);
        }
    }

    private boolean setSerialParametersLocked(final QGCUsbSerialManager.SerialParameters params) {
        if (!caps.supportsBaudRate(params.baudRate())) {
            QGCLogger.e(TAG, "Baud rate " + params.baudRate() + " is not supported for device " + address);
            return false;
        }
        return withOpenPortLocked("setting parameters", false, p -> {
            p.setParameters(params.baudRate(), params.dataBits(), params.stopBits(), params.parity());
            if (caps.needsPurgeAfterBaudChange()) {
                p.purgeHwBuffers(true, true);
            }
            currentBaudRate = params.baudRate();
            return true;
        });
    }

    @UsedFromNativeCode
    public boolean setDataTerminalReady(final boolean on) {
        synchronized (lifecycleLock) {
            return setControlLineLocked(UsbSerialPort.ControlLine.DTR, on);
        }
    }

    @UsedFromNativeCode
    public boolean setRequestToSend(final boolean on) {
        synchronized (lifecycleLock) {
            return setControlLineLocked(UsbSerialPort.ControlLine.RTS, on);
        }
    }

    private boolean setControlLineLocked(final UsbSerialPort.ControlLine controlLine, final boolean on) {
        return withOpenPortLocked("set " + controlLine, false, p -> {
            if (!isControlLineSupported(p, controlLine)) {
                QGCLogger.e(TAG, "Setting " + controlLine + " Not Supported");
                return false;
            }
            switch (controlLine) {
                case DTR: p.setDTR(on); break;
                case RTS: p.setRTS(on); break;
                default:
                    QGCLogger.w(TAG, "Setting " + controlLine + " is not supported via this method.");
                    return false;
            }
            return true;
        });
    }

    private static boolean isControlLineSupported(final UsbSerialPort p,
            final UsbSerialPort.ControlLine controlLine) {
        try {
            return p.getSupportedControlLines().contains(controlLine);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting supported control lines", e);
            return false;
        }
    }

    @UsedFromNativeCode
    public boolean setFlowControl(final int flowControl) {
        synchronized (lifecycleLock) {
            return setFlowControlLocked(flowControl);
        }
    }

    private boolean setFlowControlLocked(final int flowControl) {
        // Decode wire int via explicit switch — see #1C and SerialWireConstants.FC_*.
        final UsbSerialPort.FlowControl target;
        switch (flowControl) {
            case SerialWireConstants.FC_NONE:            target = UsbSerialPort.FlowControl.NONE; break;
            case SerialWireConstants.FC_RTS_CTS:         target = UsbSerialPort.FlowControl.RTS_CTS; break;
            case SerialWireConstants.FC_DTR_DSR:         target = UsbSerialPort.FlowControl.DTR_DSR; break;
            case SerialWireConstants.FC_XON_XOFF:        target = UsbSerialPort.FlowControl.XON_XOFF; break;
            case SerialWireConstants.FC_XON_XOFF_INLINE: target = UsbSerialPort.FlowControl.XON_XOFF_INLINE; break;
            default:
                QGCLogger.w(TAG, "Invalid flow control wire value " + flowControl);
                return false;
        }
        return withOpenPortLocked("setting Flow Control", false, p -> {
            final var supported = p.getSupportedFlowControl();
            if (supported.isEmpty() || !supported.contains(target)) {
                QGCLogger.e(TAG, "Flow Control " + target + " not supported on this port");
                return false;
            }
            if (p.getFlowControl() == target) {
                return true;
            }
            p.setFlowControl(target);
            return true;
        });
    }

    @UsedFromNativeCode
    public boolean setBreak(final boolean on) {
        synchronized (lifecycleLock) {
            return withOpenPortLocked("setting break condition", false, p -> {
                p.setBreak(on);
                return true;
            });
        }
    }

    @UsedFromNativeCode
    public boolean purgeBuffers(final boolean input, final boolean output) {
        synchronized (lifecycleLock) {
            return withOpenPortLocked("purging buffers", false, p -> {
                try {
                    p.purgeHwBuffers(input, output);
                } catch (UnsupportedOperationException ignored) {
                    // CDC-ACM drivers have no HW FIFO to purge; treat as no-op.
                }
                return true;
            });
        }
    }

    /** Non-blocking write from the C++ owner thread: hands the payload to the writer loop; returns bytes accepted or -1. */
    @UsedFromNativeCode
    public int enqueueWrite(final ByteBuffer data, final int length) {
        return writeLoop.enqueue(data, length, String.valueOf(address));
    }

    /** Ack written bytes to C++ outside lifecycleLock (mirrors emitNewData) to avoid AB-BA with the C++ writeMutex; skipped once muted so we never call into a torn-down native side. */
    private void ackBytesWritten(final long handle, final int n) {
        if (n > 0 && handle != 0 && !listenerMuted) {
            nativeBridge.deviceBytesWritten(handle, n);
        }
    }

    /** Single capability adapter handed to both loops: keeps QGCSerialPort's lifecycle/handle/port internals private while exposing each loop only the methods it needs. */
    private final class HostImpl implements SerialReadLoop.ReadHost, SerialWriteLoop.WriteHost {
        // Shared lifecycle/handle/mute/exception surface (declared on both ReadHost and WriteHost).
        @Override public Object lock() { return lifecycleLock; }
        @Override public long nativeHandleLocked() { return nativeHandle; }
        @Override public void muteListenerLocked() { listenerMuted = true; }
        @Override public void fireException(final int kind, final String message) { QGCSerialPort.this.fireException(kind, message); }

        // WriteHost
        @Override public boolean isWritableLocked() { return lifecycleState == LifecycleState.CONFIGURED; }
        @Override public int writeChunkSizeLocked() { return caps.writeChunkSizeForBaud(currentBaudRate); }
        @Override public UsbSerialPort openPortOrWarnLocked(final String operation) { return QGCSerialPort.this.openPortOrWarnLocked(operation); }
        @Override public void ackBytesWritten(final long handle, final int n) { QGCSerialPort.this.ackBytesWritten(handle, n); }
        @Override public void cancelPendingWritesUnlocked() { QGCSerialPort.this.cancelPendingWritesUnlocked(); }

        // ReadHost
        @Override public UsbSerialPort port() { return port; }
        @Override public DriverStrategy.Caps caps() { return caps; }
        @Override public int readTimeoutForIoManager() {
            // Extended (D2XX) port carries its own read-timeout constant; pass it through for that path.
            final int d2xxOverride = (extendedPort != null) ? extendedPort.readTimeoutForIoManager() : 0;
            return caps.readTimeoutForIoManager(d2xxOverride);
        }
        @Override public boolean isListenerMuted() { return listenerMuted; }
        @Override public void clearListenerMuteLocked() { listenerMuted = false; }
        @Override public void emitNewDataToNative(final long handle, final ByteBuffer buf, final int len) { nativeBridge.deviceNewData(handle, buf, len); }
        @Override public void onReaderDeviceError() {
            if (portLifecycleSink != null) {
                portLifecycleSink.onPortDeviceError(QGCSerialPort.this);
            }
        }
    }

    void fireException(final int kind, final String message) {
        // Snapshot under lock so closeLocked can't zero nativeHandle between guard and JNI call (#1B).
        final long handle;
        synchronized (lifecycleLock) {
            handle = nativeHandle;
            if (handle == 0L) return;
        }
        nativeBridge.deviceException(handle, kind, message);
    }

    private void maybeEmitDisconnectLocked(final CloseReason reason) {
        if (!reason.emitDisconnected || disconnectEmitted || nativeHandle == 0) {
            return;
        }
        disconnectEmitted = true;
        nativeBridge.deviceHasDisconnected(nativeHandle);
    }

    private UsbSerialPort openPortOrWarnLocked(final String operation) {
        if (port == null) {
            QGCLogger.d(TAG, "Attempted to " + operation + " on a null port for " + address);
            return null;
        }
        if (!port.isOpen()) {
            QGCLogger.d(TAG, "Attempted to " + operation + " on a closed port for " + address);
            return null;
        }
        return port;
    }

    @FunctionalInterface
    private interface PortOp<T> {
        T run(UsbSerialPort p) throws IOException, UnsupportedOperationException;
    }

    private <T> T withOpenPortLocked(final String operation, final T fallback, final PortOp<T> op) {
        final UsbSerialPort p = openPortOrWarnLocked(operation);
        if (p == null) {
            return fallback;
        }
        try {
            return op.run(p);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error " + operation + ": " + e);
            return fallback;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error " + operation, e);
            return fallback;
        }
    }
}
