package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialConstants.EXC_RESOURCE;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.EXC_UNKNOWN;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.MAX_NATIVE_CALLBACK_DATA_BYTES;

import org.mavlink.qgroundcontrol.QGCLogger;

import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;

/**
 * Dispatches serial data callbacks from the IO manager thread to native code.
 *
 * <p>{@link #closed} is set by {@link UsbSerialLifecycle#stopIoManager} before
 * {@code ioManager.stop()}. Both callbacks short-circuit when closed: the IO thread
 * can deliver one final batch between {@code stop()} being signalled and the thread
 * actually exiting, and we must not emit those events after the C++ side has begun
 * teardown.</p>
 */
final class QGCSerialListener implements SerialInputOutputManager.Listener {

    private static final String TAG = QGCSerialListener.class.getSimpleName();

    private final long nativeToken;
    private final UsbSerialLifecycle.Listener emitter;
    // BENCHMARK INSTRUMENTATION (debug-only). Remove with the .record() call
    // site below and the RateMonitor.java class to fully strip.
    private final RateMonitor readMonitor = new RateMonitor(TAG, "onNewData");
    private volatile boolean closed = false;

    QGCSerialListener(final long nativeToken, final UsbSerialLifecycle.Listener emitter) {
        this.nativeToken = nativeToken;
        this.emitter = emitter;
    }

    void mute() {
        closed = true;
    }

    @Override
    public void onRunError(final Exception e) {
        if (closed) return;
        // IOException at runtime is the hot-unplug / device-lost path — expected, log as Warning.
        // Everything else (RuntimeException, IllegalStateException, etc.) stays as Error.
        final boolean isUnplug = (e instanceof IOException);
        if (isUnplug) {
            QGCLogger.w(TAG, "Runner stopped (device disconnected): " + e.getMessage());
        } else {
            QGCLogger.e(TAG, "Runner stopped.", e);
        }
        final int kind = isUnplug ? EXC_RESOURCE : EXC_UNKNOWN;
        emitter.emitException(nativeToken, kind, "Runner stopped: " + e.getMessage());
    }

    @Override
    public void onNewData(final byte[] data) {
        if (closed) return;
        if (data == null || data.length == 0) {
            QGCLogger.w(TAG, "Invalid data received");
            return;
        }

        QGCLogger.v(TAG, () -> "onNewData n=" + data.length
                + " first=0x" + String.format("%02x", data[0] & 0xff));
        readMonitor.record(data.length);

        if (data.length <= MAX_NATIVE_CALLBACK_DATA_BYTES) {
            emitter.emitNewData(nativeToken, data, 0, data.length);
            return;
        }

        QGCLogger.w(TAG, "Large USB payload (" + data.length + " bytes), chunking before JNI callback");
        int offset = 0;
        while (offset < data.length) {
            final int chunkLen = Math.min(MAX_NATIVE_CALLBACK_DATA_BYTES, data.length - offset);
            emitter.emitNewData(nativeToken, data, offset, chunkLen);
            offset += chunkLen;
        }
    }
}
