package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Stateless wrapper around the per-port operations exposed via JNI: write,
 * parameter / control-line / flow-control mutators, break, and buffer purge.
 *
 * <p>All instance methods take a {@code deviceId} and resolve it through the
 * supplied {@link PortResolver}. This keeps the resource-registry plumbing out
 * of this class — the bridge never sees the registry directly.</p>
 */
final class UsbSerialIoBridge {

    private static final String TAG = UsbSerialIoBridge.class.getSimpleName();

    /** Max single-write payload we expect (covers a MAVLink v2 max + headroom). */
    private static final int WRITE_SCRATCH_BYTES = 16384;
    private static final int WRITE_SCRATCH_POOL_CAP = 4;

    /** Single-direction lookup the bridge needs to perform any operation. */
    interface PortResolver {
        /** Returns the open port for {@code deviceId}, or null (after warning) if missing/closed. */
        UsbSerialPort openPortOrWarn(int deviceId, String operation);
        /** Returns the IO manager for {@code deviceId}, or null if not registered. */
        SerialInputOutputManager ioManager(int deviceId);
        /** Returns the async write pump for {@code deviceId}, or null if pump-less (FTDI VCP, init failure). */
        AsyncUsbWritePump writePump(int deviceId);
    }

    @FunctionalInterface
    private interface PortOp<T> {
        T run(UsbSerialPort port) throws IOException, UnsupportedOperationException;
    }

    private final PortResolver resolver;

    // port.write/writeAsync take byte[], so the heap copy is unavoidable; the allocation is not.
    private final BoundedPool<byte[]> writeScratchPool = new BoundedPool<>(
            WRITE_SCRATCH_POOL_CAP,
            length -> new byte[Math.max(WRITE_SCRATCH_BYTES, length)],
            buf -> buf.length);

    // BENCHMARK INSTRUMENTATION (debug-only). Remove these fields, the
    // RateMonitor.java class, and the .record() call sites to fully strip.
    private final RateMonitor writeSyncMonitor = new RateMonitor(TAG, "writeDirect");
    private final RateMonitor writeAsyncMonitor = new RateMonitor(TAG, "writeAsyncDirect");

    UsbSerialIoBridge(final PortResolver resolver) {
        this.resolver = resolver;
    }

    private <T> T withOpenPort(final int deviceId, final String operation, final T fallback,
            final PortOp<T> op) {
        final UsbSerialPort port = resolver.openPortOrWarn(deviceId, operation);
        if (port == null) {
            return fallback;
        }
        try {
            return op.run(port);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error " + operation + ": " + e);
            return fallback;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error " + operation, e);
            return fallback;
        }
    }

    // data.hasArray() is always false for direct ByteBuffers — must transfer into heap byte[].
    int writeDirect(final int deviceId, final ByteBuffer data, final int length, final int timeoutMSec) {
        final byte[] heap = writeScratchPool.borrow(length);
        data.position(0);
        data.get(heap, 0, length);
        QGCLogger.v(TAG, () -> "writeDirect deviceId=" + deviceId + " n=" + length
                + " first=0x" + String.format("%02x", heap[0] & 0xff));
        writeSyncMonitor.record(length);
        // Async pump path: pump takes ownership of `heap` until its worker finishes,
        // then fires the release callback. Eliminates the pump's prior per-submit
        // System.arraycopy into a queue-owned byte[] — the buffer is enqueued in-place.
        // Falls through to mik3y blocking write when pump is null (FTDI VCP / init failure).
        final AsyncUsbWritePump pump = resolver.writePump(deviceId);
        if (pump != null) {
            return pump.submit(heap, length, () -> writeScratchPool.release(heap));
        }
        try {
            return withOpenPort(deviceId, "write", -1, port -> {
                try {
                    port.write(heap, length, timeoutMSec);
                    return length;
                } catch (final SerialTimeoutException e) {
                    QGCLogger.e(TAG, "Write timeout occurred", e);
                    return -1;
                }
            });
        } finally {
            writeScratchPool.release(heap);
        }
    }

    int writeAsyncDirect(final int deviceId, final ByteBuffer data, final int length, final int timeoutMSec) {
        final SerialInputOutputManager io = resolver.ioManager(deviceId);
        if (io == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return -1;
        }
        // writeAsync retains the byte[] reference; must not pool it.
        final byte[] heap = new byte[length];
        data.position(0);
        data.get(heap, 0, length);
        QGCLogger.v(TAG, () -> "writeAsyncDirect deviceId=" + deviceId + " n=" + length
                + " first=0x" + String.format("%02x", heap[0] & 0xff));
        writeAsyncMonitor.record(length);
        io.setWriteTimeout(timeoutMSec);
        io.writeAsync(heap);
        return length;
    }

    boolean setParameters(final int deviceId, final int baudRate, final int dataBits,
            final int stopBits, final int parity) {
        return withOpenPort(deviceId, "setting parameters", false, port -> {
            port.setParameters(baudRate, dataBits, stopBits, parity);
            return true;
        });
    }

    boolean setControlLine(final int deviceId, final UsbSerialPort.ControlLine controlLine, final boolean on) {
        return withOpenPort(deviceId, "set " + controlLine, false, port -> {
            if (!isControlLineSupported(port, controlLine)) {
                QGCLogger.e(TAG, "Setting " + controlLine + " Not Supported");
                return false;
            }
            switch (controlLine) {
                case DTR: port.setDTR(on); break;
                case RTS: port.setRTS(on); break;
                default:
                    QGCLogger.w(TAG, "Setting " + controlLine + " is not supported via this method.");
                    return false;
            }
            return true;
        });
    }

    private static boolean isControlLineSupported(final UsbSerialPort port,
            final UsbSerialPort.ControlLine controlLine) {
        try {
            return port.getSupportedControlLines().contains(controlLine);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting supported control lines", e);
            return false;
        }
    }

    int[] getControlLines(final int deviceId) {
        return withOpenPort(deviceId, "getting control lines", new int[]{}, port ->
                port.getControlLines().stream()
                        .mapToInt(UsbSerialPort.ControlLine::ordinal)
                        .toArray());
    }

    int getFlowControl(final int deviceId) {
        final UsbSerialPort port = resolver.openPortOrWarn(deviceId, "get flow control");
        if (port == null) {
            return 0;
        }
        if (port.getSupportedFlowControl().isEmpty()) {
            QGCLogger.e(TAG, "Flow Control Not Supported");
            return 0;
        }
        return port.getFlowControl().ordinal();
    }

    boolean setFlowControl(final int deviceId, final int flowControl) {
        if (flowControl < 0 || flowControl >= UsbSerialPort.FlowControl.values().length) {
            QGCLogger.w(TAG, "Invalid flow control ordinal " + flowControl);
            return false;
        }
        final UsbSerialPort.FlowControl target = UsbSerialPort.FlowControl.values()[flowControl];
        return withOpenPort(deviceId, "setting Flow Control", false, port -> {
            final var supported = port.getSupportedFlowControl();
            if (supported.isEmpty() || !supported.contains(target)) {
                QGCLogger.e(TAG, "Flow Control " + target + " not supported on this port");
                return false;
            }
            if (port.getFlowControl() == target) {
                return true;
            }
            port.setFlowControl(target);
            return true;
        });
    }

    boolean setBreak(final int deviceId, final boolean on) {
        return withOpenPort(deviceId, "setting break condition", false, port -> {
            port.setBreak(on);
            return true;
        });
    }

    boolean purgeBuffers(final int deviceId, final boolean input, final boolean output) {
        return withOpenPort(deviceId, "purging buffers", false, port -> {
            try {
                port.purgeHwBuffers(input, output);
            } catch (UnsupportedOperationException ignored) {
                // CDC-ACM drivers (Orange Cube et al.) have no HW FIFO to purge — treat as no-op.
            }
            return true;
        });
    }
}
