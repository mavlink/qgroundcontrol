package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.EXC_RESOURCE;
import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.EXC_UNKNOWN;
import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.MAX_CHUNK_BYTES;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbEndpoint;
import android.os.Process;

import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Drains the RX path on hoho's {@link SerialInputOutputManager} reader thread, forwarding device bytes to C++.
 *
 * <p>Mirror of {@link SerialWriteLoop}: all state shared with the owning port (lifecycle, native handle, mute flag) is
 * read under {@link ReadHost#lock()}, and the JNI emit runs outside that lock so it can never deadlock a C++ close path.
 * The stop handshake parks under the same monitor until the reader reaches STOPPED, because {@code port.close()} during
 * a concurrent read crashes FTDI D2XX.</p>
 */
final class SerialReadLoop implements SerialInputOutputManager.Listener {

    private static final String TAG = SerialReadLoop.class.getSimpleName();

    /** Default read buffer size handed to {@code SerialInputOutputManager}. */
    private static final int READ_BUF_SIZE = 2048;
    /** Serial package-wide verbose tag; enables hot-path serial diagnostics with one setprop. */
    private static final String VERBOSE_LOGGING = "QGCSerial";

    // Budget for stopIoManagerLocked() to see the reader reach STOPPED; exceeds the reader's blocking read window so a parked reader can wake and transition.
    private static final long STOP_HANDSHAKE_TIMEOUT_NS = 500_000_000L;  // 500 ms
    private static final long STOP_HANDSHAKE_POLL_MS    = 25L;

    /** Narrow seam onto the owning port: the loop reads lifecycle/handle/mute under {@link #lock()} and drives native emits/exceptions through it. */
    interface ReadHost extends PortMonitor {
        /** The open port for this read loop, or null. */
        UsbSerialPort port();
        /** Per-driver quirks for read buffer/queue sizing. */
        DriverStrategy.Caps caps();
        /** D2XX port carries its own read-timeout constant; the host passes it through for the D2XX path. */
        int readTimeoutForIoManager();
        boolean isListenerMuted();
        /** Caller holds {@link #lock()}: clear the mute so a freshly (re)opened reader may emit again. */
        void clearListenerMuteLocked();
        /** RX forward into the facade's NativeBridge (shared by read+write+exception); called outside {@link #lock()}. */
        void emitNewDataToNative(long handle, ByteBuffer buf, int len);
        /** Hot-unplug (onRunError IOException, non-timeout) → tell the lifecycle sink the device is gone (#1A). */
        void onReaderDeviceError();
    }

    private final ReadHost host;
    private final Object lock;
    private SerialInputOutputManager ioManager;
    // Direct buffer ferrying read bytes to native; written only by the per-port read thread (onNewData), so the emit is lock-free.
    private final ByteBuffer nativeDataBuffer = ByteBuffer.allocateDirect(MAX_CHUNK_BYTES);

    SerialReadLoop(final ReadHost host) {
        this.host = host;
        this.lock = host.lock();
    }

    /** Caller holds {@link ReadHost#lock()}. Builds and configures the reader; idempotent while one already exists. */
    boolean createIoManagerLocked() {
        if (ioManager != null) {
            return true;
        }
        final UsbSerialPort port = host.port();
        if (port == null) {
            return false;
        }

        host.clearListenerMuteLocked();
        ioManager = new SerialInputOutputManager(port, this);

        int readBufferSize = READ_BUF_SIZE;
        final UsbEndpoint readEndpoint = port.getReadEndpoint();
        if (readEndpoint != null) {
            readBufferSize = Math.max(readEndpoint.getMaxPacketSize(), READ_BUF_SIZE);
        }
        ioManager.setReadBufferSize(readBufferSize);

        try {
            ioManager.setReadTimeout(host.readTimeoutForIoManager());
            ioManager.setReadQueue(host.caps().readQueueDepth());
            ioManager.setWriteTimeout(0);
            ioManager.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager configuration error:", e);
            return false;
        }

        return true;
    }

    /** Caller holds {@link ReadHost#lock()}. Starts the reader thread; refuses if no IO manager exists. */
    boolean startLocked(final QGCUsbSerialManager.PortAddress address) {
        if (ioManager == null) {
            QGCLogger.w(TAG, "IO Manager not found for " + address);
            return false;
        }
        if (ioManager.getState() == SerialInputOutputManager.State.RUNNING) {
            return true;
        }
        try {
            ioManager.start();
            return true;
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager Start exception:", e);
            return false;
        }
    }

    /** Caller holds {@link ReadHost#lock()}. Posts STOPPING, then releases the monitor via wait()/poll until the reader reaches STOPPED. */
    boolean stopIoManagerLocked(final QGCUsbSerialManager.PortAddress address) {
        host.muteListenerLocked();
        if (ioManager == null) {
            return false;
        }
        SerialInputOutputManager.State ioState = ioManager.getState();
        if (ioState != SerialInputOutputManager.State.STOPPED
                && ioState != SerialInputOutputManager.State.STOPPING) {
            ioManager.stop();
        }
        // Block until the reader reaches STOPPED — closing the port mid-read crashes FTDI D2XX.
        // wait() releases the host lock so a muted onNewData can exit and let the reader transition.
        final long deadlineNs = System.nanoTime() + STOP_HANDSHAKE_TIMEOUT_NS;
        while (ioManager.getState() != SerialInputOutputManager.State.STOPPED) {
            final long remainingMs = (deadlineNs - System.nanoTime()) / 1_000_000L;
            if (remainingMs <= 0) {
                QGCLogger.w(TAG, "IO manager stop handshake timed out for " + address
                        + " (state=" + ioManager.getState() + ")");
                break;
            }
            try {
                // Nothing notifies; the timeout wakes us.
                lock.wait(Math.min(remainingMs, STOP_HANDSHAKE_POLL_MS));
            } catch (final InterruptedException ie) {
                Thread.currentThread().interrupt();
                break;
            }
        }
        // Drop the reference once stopped so startLocked()'s null-check can't race a STOPPING SerialInputOutputManager being re-started.
        ioManager = null;
        return true;
    }

    @Override
    public void onRunError(final Exception e) {
        if (host.isListenerMuted()) return;
        // Runtime IOException is the hot-unplug path; a SerialTimeoutException is a still-valid stall (not unplug), excluded so a transient timeout doesn't replace the driver via onPortDeviceError.
        final boolean isUnplug = (e instanceof IOException) && !(e instanceof SerialTimeoutException);
        if (isUnplug) {
            QGCLogger.w(TAG, "Runner stopped (device disconnected): " + e.getMessage());
        } else {
            QGCLogger.e(TAG, "Runner stopped.", e);
        }
        final int kind = isUnplug ? EXC_RESOURCE : EXC_UNKNOWN;
        host.fireException(kind, "Runner stopped: " + e.getMessage());
        if (isUnplug) {
            host.onReaderDeviceError();
        }
    }

    @Override
    public void onNewData(final byte[] data) {
        if (data == null || data.length == 0) {
            QGCLogger.w(TAG, "Invalid data received");
            return;
        }

        QGCLogger.v(TAG, VERBOSE_LOGGING, () -> "onNewData n=" + data.length
                + " first=0x" + String.format("%02x", data[0] & 0xff));

        // Read the mute flag and snapshot nativeHandle under the host lock so closeLocked can't zero it
        // between guard and JNI call (#1B); holding the lock across emitNewData would deadlock C++ close paths.
        final long handle;
        synchronized (lock) {
            if (host.isListenerMuted()) return;
            handle = host.nativeHandleLocked();
            if (handle == 0L) return;
        }
        if (data.length <= MAX_CHUNK_BYTES) {
            emitNewData(handle, data, 0, data.length);
            return;
        }

        QGCLogger.w(TAG, "Large USB payload (" + data.length + " bytes), chunking before JNI callback");
        int offset = 0;
        while (offset < data.length) {
            final int chunkLen = Math.min(MAX_CHUNK_BYTES, data.length - offset);
            emitNewData(handle, data, offset, chunkLen);
            offset += chunkLen;
        }
    }

    private void emitNewData(final long nativeToken, final byte[] data, final int offset, final int length) {
        // No lock: nativeDataBuffer is touched only by the single per-port read thread (onNewData).
        nativeDataBuffer.clear();
        nativeDataBuffer.put(data, offset, length);
        nativeDataBuffer.flip();
        host.emitNewDataToNative(nativeToken, nativeDataBuffer, length);
    }
}
