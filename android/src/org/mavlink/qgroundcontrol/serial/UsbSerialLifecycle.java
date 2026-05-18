package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialConstants.BAD_DEVICE_ID;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.EXC_OPEN_FAILED;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.EXC_RESOURCE;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.EXC_UNKNOWN;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.FTDI_CTRL_TIMEOUT_MS;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.FTDI_LATENCY_MS;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.FTDI_REQTYPE_OUT;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.FTDI_SIO_SET_LATENCY;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.MAX_NATIVE_CALLBACK_DATA_BYTES;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.READ_BUF_SIZE;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.READ_QUEUE_DEPTH;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbManager;
import android.os.Process;

import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;

/**
 * Owns the per-port open/close flow and the SerialInputOutputManager start/stop
 * lifecycle. State (UsbDeviceRegistry, UsbManager, enumerator) is injected by the
 * facade; native-emission is funneled through {@link Listener} so this class
 * never depends on JNI directly.
 */
final class UsbSerialLifecycle {

    private static final String TAG = UsbSerialLifecycle.class.getSimpleName();

    /** FTDI ports open/close the {@link UsbDeviceConnection} themselves; the manager
     *  must not store a second reference. CDC and other upstream drivers do not. */
    private static boolean ftdiOwnsConnection(final UsbSerialDriver driver) {
        return driver instanceof QGCFtdiSerialDriver;
    }

    /** Outbound channel for native-side notifications. Implemented by the manager facade. */
    interface Listener {
        void emitException(long nativeToken, int kind, String message);
        void emitNewData(long nativeToken, byte[] data, int offset, int length);
    }

    private final UsbManager usbManager;
    private final UsbDeviceRegistry registry;
    private final UsbSerialEnumerator enumerator;
    private final Listener listener;

    UsbSerialLifecycle(final UsbManager usbManager,
            final UsbDeviceRegistry registry,
            final UsbSerialEnumerator enumerator,
            final Listener listener) {
        this.usbManager = usbManager;
        this.registry = registry;
        this.enumerator = enumerator;
        this.listener = listener;
    }

    /** Closes every port whose registry entry maps to {@code physicalDeviceId}. */
    void removeAllResourcesForPhysicalDevice(final int physicalDeviceId) {
        for (final int handle : registry.handlesForPhysicalDevice(physicalDeviceId)) {
            close(handle);
        }
    }

    /** Looks up the port for a registered handle, or null. Shared with IoBridge resolver.
     *  Logs at debug — null lookups are normal during the detach→close race when C++ has
     *  pending writes still draining as the registry entry is being removed. */
    UsbSerialPort findPortByDeviceId(final int deviceId) {
        if (deviceId == BAD_DEVICE_ID) {
            QGCLogger.w(TAG, "Finding port failed for invalid Device ID " + deviceId);
            return null;
        }
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null || res.driver() == null) {
            QGCLogger.d(TAG, "No resources found for device ID " + deviceId);
            return null;
        }
        final UsbSerialPort port = UsbSerialEnumerator.getPortFromDriver(res.driver(), res.portIndex());
        if (port == null) {
            QGCLogger.d(TAG, "No port available on device ID " + deviceId
                    + " at port index " + res.portIndex());
            return null;
        }
        return port;
    }

    int open(final String deviceName, final long nativeToken) {
        final UsbSerialEnumerator.DevicePortSpec spec = UsbSerialEnumerator.parseDevicePortSpec(deviceName);
        final UsbSerialDriver driver = enumerator.findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            // Common during BL→app fw transitions: device path changes mid-reconnect. C++ side
            // surfaces DeviceNotFoundError, so this is debug-only to avoid duplicate noise.
            QGCLogger.d(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final UsbDevice device = driver.getDevice();
        final UsbSerialPort port = UsbSerialEnumerator.getPortFromDriver(driver, spec.portIndex);
        if (port == null) {
            QGCLogger.w(TAG, "No port " + spec.portIndex + " available on device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final int physicalDeviceId = device.getDeviceId();
        final int resourceId = registry.getOrCreateHandle(physicalDeviceId, spec.portIndex);

        final UsbDeviceRegistry.UsbDeviceResources existing = registry.getByHandle(resourceId);
        if (existing == null) {
            QGCLogger.e(TAG, "Resource entry missing for resourceId " + resourceId);
            return BAD_DEVICE_ID;
        }
        // Re-opening an entry already past REGISTERED means a previous open didn't roll back fully;
        // treat as fatal rather than silently overwriting state.
        if (existing.state() != UsbDeviceRegistry.PortLifecycleState.REGISTERED) {
            QGCLogger.w(TAG, "Open requested on handle " + resourceId + " already in state "
                    + existing.state() + "; refusing to reopen");
            return BAD_DEVICE_ID;
        }
        // openDriver emits its own EXC_OPEN_FAILED on each failure; rollback does not re-emit.
        if (!openDriver(port, device, resourceId, driver, spec.portIndex, nativeToken)) {
            QGCLogger.e(TAG, "Failed to open driver for device " + deviceName);
            return rollbackOpen(resourceId, null);
        }

        if (!createIoManager(resourceId, port, nativeToken)) {
            listener.emitException(nativeToken, EXC_OPEN_FAILED,
                    "Failed to start I/O for device: " + deviceName);
            return rollbackOpen(resourceId, port);
        }

        QGCLogger.d(TAG, "Port open successful: " + port.toString());
        return resourceId;
    }

    /** Tears down a partial open and returns BAD_DEVICE_ID. {@code port} may be null
     *  when openDriver failed before the port was usable; closeAndClearConnection is
     *  always safe (no-op when no connection was stored, e.g. FTDI-owned). */
    private int rollbackOpen(final int handle, final UsbSerialPort port) {
        if (port != null) {
            try {
                port.close();
            } catch (final IOException e) {
                QGCLogger.e(TAG, "Error closing port during rollback", e);
            }
        }
        closeAndClearConnection(handle);
        registry.remove(handle);
        return BAD_DEVICE_ID;
    }

    private boolean openDriver(final UsbSerialPort port, final UsbDevice device,
            final int deviceId, final UsbSerialDriver driver, final int portIndex,
            final long nativeToken) {
        if (port == null) {
            QGCLogger.w(TAG, "Null UsbSerialPort for device " + device.getDeviceName());
            listener.emitException(nativeToken, EXC_OPEN_FAILED,
                    "No serial port available for device: " + device.getDeviceName());
            return false;
        }

        if (port.isOpen()) {
            QGCLogger.d(TAG, "Port already open for device ID " + deviceId);
            return true;
        }

        final UsbDeviceConnection connection = usbManager.openDevice(device);
        if (connection == null) {
            QGCLogger.w(TAG, "No Usb Device Connection");
            listener.emitException(nativeToken, EXC_OPEN_FAILED,
                    "No USB device connection for device: " + device.getDeviceName());
            return false;
        }

        try {
            port.open(connection);
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error opening driver for device " + device.getDeviceName(), ex);
            listener.emitException(nativeToken, EXC_OPEN_FAILED,
                    "Error opening driver: " + ex.getMessage());
            connection.close();
            return false;
        }

        // VCP-mode FTDI: drop chip latency timer from default 16ms so short MAVLink
        // reads aren't held by the chip's batching window. wIndex is the 1-based
        // port number (1 for single-port chips; A=1/B=2 on FT2232H, etc.). D2XX
        // path handles its own latency timer inside QGCFtdiSerialPort.open().
        if (driver instanceof FtdiSerialDriver) {
            try {
                connection.controlTransfer(FTDI_REQTYPE_OUT, FTDI_SIO_SET_LATENCY,
                        FTDI_LATENCY_MS, portIndex + 1, null, 0, FTDI_CTRL_TIMEOUT_MS);
            } catch (final Throwable t) {
                QGCLogger.w(TAG, "FTDI setLatencyTimer failed: " + t.getMessage());
            }
        }

        // QGCFtdiSerialDriver takes ownership of the UsbDeviceConnection inside port.open()
        // and closes it from port.close(); storing a second reference here causes a double-close.
        final boolean ftdiOwnsConn = ftdiOwnsConnection(driver);
        final UsbDeviceConnection storedConn = ftdiOwnsConn ? null : connection;
        // Async write pump pipelines outbound bytes. Two construction paths:
        //   bulkTransfer: for kernel-claimed interfaces (CDC-ACM, CP210x, CH34x, mik3y FTDI).
        //   D2XX: for QGCFtdiSerialDriver — D2XX claims the FTDI interface exclusively on its
        //   own connection, so we route writes through FT_Device.write(wait=true) via a
        //   single-worker pump with baud-aware throttling.
        AsyncUsbWritePump pump = null;
        try {
            if (port instanceof QGCFtdiSerialDriver.QGCFtdiSerialPort) {
                final QGCFtdiSerialDriver.QGCFtdiSerialPort ftdiPort =
                        (QGCFtdiSerialDriver.QGCFtdiSerialPort) port;
                pump = AsyncUsbWritePump.forD2xx(ftdiPort.asyncWriteOp());
                final AsyncUsbWritePump finalPump = pump;
                ftdiPort.setBaudListener(finalPump::setBaudRate);
                QGCLogger.d(TAG, "AsyncUsbWritePump started (D2XX baud-throttled) for device ID " + deviceId);
            } else {
                final UsbEndpoint writeEp = port.getWriteEndpoint();
                if (writeEp != null) {
                    pump = AsyncUsbWritePump.forBulkTransfer(connection, writeEp);
                    QGCLogger.d(TAG, "AsyncUsbWritePump started (bulkTransfer) for device ID " + deviceId);
                }
            }
        } catch (final Throwable t) {
            QGCLogger.w(TAG, "AsyncUsbWritePump init failed; falling back to blocking write: " + t.getMessage());
            pump = null;
        }
        final int fd = connection.getFileDescriptor();
        if (registry.getByHandle(deviceId) != null) {
            final AsyncUsbWritePump finalPump = pump;
            registry.updateByHandle(deviceId, r ->
                    r.withOpenState(driver, portIndex, nativeToken, fd, storedConn, finalPump));
        } else {
            // getOrCreateHandle always registers; close to avoid leaking the connection if missed.
            if (pump != null) pump.close();
            connection.close();
        }

        QGCLogger.d(TAG, "Port Driver open successful");
        return true;
    }

    private void closeAndClearConnection(final int handle) {
        // Pump owns UsbRequests bound to the connection; must close before the connection.
        final AsyncUsbWritePump pump = registry.extractWritePump(handle);
        if (pump != null) {
            try { pump.close(); }
            catch (final Throwable t) { QGCLogger.w(TAG, "Error closing AsyncUsbWritePump: " + t.getMessage()); }
        }
        final UsbDeviceConnection conn = registry.extractConnection(handle);
        if (conn != null) {
            try { conn.close(); }
            catch (final Throwable t) { QGCLogger.w(TAG, "Error closing UsbDeviceConnection: " + t.getMessage()); }
        }
    }

    private boolean createIoManager(final int deviceId, final UsbSerialPort port,
            final long nativeToken) {
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return false;
        }

        if (res.ioManager() != null) {
            QGCLogger.i(TAG, "IO Manager already exists for device ID " + deviceId);
            return true;
        }

        if (port == null) {
            QGCLogger.w(TAG, "Cannot create USB serial IO manager with null port for device ID "
                    + deviceId);
            return false;
        }

        final QGCSerialListener serialListener = new QGCSerialListener(nativeToken, listener);
        final SerialInputOutputManager ioManager = new SerialInputOutputManager(port, serialListener);

        int readBufferSize = READ_BUF_SIZE;
        final UsbEndpoint readEndpoint = port.getReadEndpoint();
        if (readEndpoint != null) {
            readBufferSize = Math.max(readEndpoint.getMaxPacketSize(), READ_BUF_SIZE);
        }
        ioManager.setReadBufferSize(readBufferSize);

        QGCLogger.d(TAG, "Read Buffer Size: " + ioManager.getReadBufferSize());
        QGCLogger.d(TAG, "Write Buffer Size: " + ioManager.getWriteBufferSize());

        try {
            ioManager.setReadTimeout(0);
            ioManager.setReadQueue(READ_QUEUE_DEPTH);
            ioManager.setWriteTimeout(0);
            ioManager.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager configuration error:", e);
            return false;
        }

        registry.updateByHandle(deviceId, r -> r.withIoReady(ioManager, serialListener));
        QGCLogger.d(TAG, "Serial I/O Manager created for device ID " + deviceId);
        return true;
    }

    boolean startIoManager(final int deviceId) {
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null || res.ioManager() == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return false;
        }

        final SerialInputOutputManager.State ioState = res.ioManager().getState();
        if (ioState == SerialInputOutputManager.State.RUNNING) {
            return true;
        }

        try {
            res.ioManager().start();
            QGCLogger.d(TAG, "Serial I/O Manager started for device ID " + deviceId);
            return true;
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager Start exception:", e);
            return false;
        }
    }

    boolean stopIoManager(final int deviceId) {
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null || res.ioManager() == null) {
            return false;
        }

        final SerialInputOutputManager.State ioState = res.ioManager().getState();
        if (ioState == SerialInputOutputManager.State.STOPPED
                || ioState == SerialInputOutputManager.State.STOPPING) {
            return true;
        }

        // Mute before stop(): the IO thread can deliver one final batch between the signal
        // and actual exit; those events must not reach C++ after teardown begins.
        if (res.listener() != null) {
            res.listener().mute();
        }

        res.ioManager().stop(); // fire-and-return; stale callbacks are harmless (listener muted, token no-ops)
        QGCLogger.d(TAG, "Serial I/O Manager stop signalled for device ID " + deviceId);
        return true;
    }

    boolean ioManagerRunning(final int deviceId) {
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null || res.ioManager() == null) {
            return false;
        }
        return res.ioManager().getState() == SerialInputOutputManager.State.RUNNING;
    }

    boolean close(final int deviceId) {
        final UsbDeviceRegistry.UsbDeviceResources res = registry.getByHandle(deviceId);
        if (res == null) {
            QGCLogger.d(TAG, "Close requested for already cleaned device ID " + deviceId);
            return true;
        }
        if (res.state() == UsbDeviceRegistry.PortLifecycleState.CLOSING) {
            // Another thread is already tearing down — let it finish; this call is a no-op.
            QGCLogger.d(TAG, "Close already in progress for device ID " + deviceId);
            return true;
        }
        registry.updateByHandle(deviceId, r -> r.withState(UsbDeviceRegistry.PortLifecycleState.CLOSING));

        // port.isOpen()==false does NOT mean IO thread has exited — always stop first.
        stopIoManager(deviceId);

        // Pump cancels in-flight UsbRequests; must run before port.close() releases the interface.
        final AsyncUsbWritePump pump = registry.extractWritePump(deviceId);
        if (pump != null) {
            try { pump.close(); }
            catch (final Throwable t) { QGCLogger.w(TAG, "Error closing AsyncUsbWritePump: " + t.getMessage()); }
        }

        final UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null || !port.isOpen()) {
            if (port == null) {
                QGCLogger.w(TAG, "Attempted to close a null port for device ID " + deviceId);
            } else {
                QGCLogger.d(TAG, "Close requested for already closed device ID " + deviceId);
            }
            closeAndClearConnection(deviceId);
            registry.remove(deviceId);
            return true;
        }

        try {
            port.close();
            QGCLogger.d(TAG, "Device " + deviceId + " closed successfully.");
            return true;
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error closing driver:", ex);
            return false;
        } finally {
            // Release the fd before removing the registry entry; FTDI path stored null so this is a no-op there.
            closeAndClearConnection(deviceId);
            registry.remove(deviceId);
        }
    }

}
