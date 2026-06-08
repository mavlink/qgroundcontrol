package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialWireConstants.EXC_RESOURCE;

import org.mavlink.qgroundcontrol.QGCLogger;

import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Drains queued writes on a dedicated thread, acking each device-sized sub-write to C++ for backpressure accounting.
 *
 * <p>Synchronous {@code port.write()} (not {@code ioManager.writeAsync}) lets every accepted byte be acked via
 * {@link WriteHost#ackBytesWritten}. All state shared with the owning port (lifecycle, baud, native handle) is read
 * under {@link WriteHost#lock()} so critical sections stay identical to the in-port original.</p>
 */
final class SerialWriteLoop {

    private static final String TAG = SerialWriteLoop.class.getSimpleName();

    private static final byte[] WRITER_STOP = new byte[0];
    private static final int WRITER_WRITE_TIMEOUT_MS = 5000;
    private static final int WRITER_WRITE_MAX_TIMEOUT_RETRIES = 2;
    // Short so teardown never outlasts the C++ DISCONNECT_TIMEOUT (3s); a wedged writer is unblocked by the close right after this join, not by waiting it out.
    private static final long WRITER_JOIN_TIMEOUT_MS = 1000L;
    private static final long ENQUEUE_OFFER_TIMEOUT_NS = 2_000_000_000L; // 2s < C++ DISCONNECT_TIMEOUT (3s)
    static final int WRITE_QUEUE_CAPACITY = 64;

    /** Narrow seam onto the owning port: the loop reads lifecycle/handle/caps under {@link #lock()} and drives native acks/exceptions through it. */
    interface WriteHost extends PortMonitor {
        /** Caller holds {@link #lock()}: true while the port may still accept writes (CONFIGURED). */
        boolean isWritableLocked();
        /** Caller holds {@link #lock()}: device-sized sub-write length for the current baud. */
        int writeChunkSizeLocked();
        /** Caller holds {@link #lock()}: the open port, or null (warns) if unavailable. */
        UsbSerialPort openPortOrWarnLocked(String operation);
        void ackBytesWritten(long handle, int n);
        void cancelPendingWritesUnlocked();
    }

    private final WriteHost host;
    private final Object lock;
    private final LinkedBlockingQueue<byte[]> writeQueue = new LinkedBlockingQueue<>(WRITE_QUEUE_CAPACITY);
    private volatile Thread writerThread;
    /** Writers that overran their join timeout; retained so a reopen fails fast while any is still alive. */
    private final List<Thread> leakedWriterThreads = new ArrayList<>();
    private volatile boolean writerRunning;

    SerialWriteLoop(final WriteHost host) {
        this.host = host;
        this.lock = host.lock();
    }

    boolean isRunning() {
        return writerRunning;
    }

    /** Test-only: joins the active and any leaked writer threads so assertions never race a winding-down writer. */
    boolean awaitDrainedForTest(final long timeoutMs) throws InterruptedException {
        final List<Thread> pending = new ArrayList<>();
        synchronized (lock) {
            if (writerThread != null) {
                pending.add(writerThread);
            }
            pending.addAll(leakedWriterThreads);
        }
        boolean drained = true;
        for (final Thread t : pending) {
            t.join(timeoutMs);
            if (t.isAlive()) {
                drained = false;
            }
        }
        return drained;
    }

    /** Caller holds {@link WriteHost#lock()}. Starts the writer thread; refuses if a prior writer is still stuck. */
    boolean startLocked(final String address) {
        if (writerThread != null && writerThread.isAlive()) {
            return true;
        }
        // A writer that self-exited (e.g. fireException) leaves the field non-null but dead; clear it so a fresh thread starts instead of silently dropping TX.
        writerThread = null;
        if (!leakedWriterThreads.isEmpty()) {
            for (final Thread leaked : leakedWriterThreads) {
                if (leaked.isAlive()) {
                    QGCLogger.e(TAG, "Cannot start writer for " + address
                            + "; a prior writer is still stuck (leaked). Refusing to claim TX is live.");
                    return false;
                }
            }
            QGCLogger.i(TAG, "All previously leaked writers for " + address + " have exited; clearing records");
            leakedWriterThreads.clear();
        }
        writeQueue.clear();
        writerRunning = true;
        final Thread t = new Thread(() -> runWriteLoop(address), "QGCSerialWriter");
        t.setDaemon(true);
        writerThread = t;
        t.start();
        return true;
    }

    /** Stops the writer thread and joins it; caller must NOT hold {@link WriteHost#lock()}. */
    void stopUnlocked(final String address) {
        final Thread t = writerThread;
        if (t == null) {
            return;
        }
        synchronized (lock) {
            writerRunning = false;
            host.muteListenerLocked();
            writeQueue.clear();
            writeQueue.offer(WRITER_STOP);
        }
        host.cancelPendingWritesUnlocked();
        try {
            t.join(WRITER_JOIN_TIMEOUT_MS);
        } catch (final InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        synchronized (lock) {
            if (t.isAlive()) {
                QGCLogger.e(TAG, "Writer thread for " + address
                        + " did not exit within " + WRITER_JOIN_TIMEOUT_MS + "ms; leaking reference");
                // Park the stuck thread off the active field so a reopen can try a fresh writer; startLocked() re-checks every parked writer and fails loudly if any is still hung.
                leakedWriterThreads.add(t);
                writerThread = null;
            } else {
                writerThread = null;
            }
        }
    }

    /** Non-blocking enqueue from the C++ owner thread; copies into a fresh array for the writer thread and returns bytes accepted. */
    int enqueue(final ByteBuffer data, final int length, final String address) {
        if (data == null || length <= 0) {
            QGCLogger.w(TAG, "Invalid write request for " + address);
            return -1;
        }
        // Unique per call: writeQueue.remove(chunk) on the close path relies on this array identity.
        final byte[] chunk = new byte[length];
        data.position(0);
        data.get(chunk, 0, length);

        // Bounded blocking enqueue: a full queue is backpressure, not fatal — block briefly so the writer drains rather than returning the link-fatal sentinel; only a non-writable lifecycle returns -1.
        synchronized (lock) {
            if (!host.isWritableLocked() || !writerRunning) {
                QGCLogger.w(TAG, "enqueueWrite rejected in non-writable state for " + address);
                return -1;
            }
        }
        try {
            // Single bounded offer outside the lock (the writer needs the lock to advance subwrites); stopUnlocked always clear()s on teardown, so a full queue drains within the timeout if the port is still live.
            if (!writeQueue.offer(chunk, ENQUEUE_OFFER_TIMEOUT_NS, TimeUnit.NANOSECONDS)) {
                QGCLogger.w(TAG, "Write queue saturated past timeout for " + address);
                return -1;
            }
            // Re-check under lock: if stopUnlocked ran clear()+offer(WRITER_STOP) after our offer, remove our payload so it can't drain after WRITER_STOP and return the failed-enqueue sentinel.
            synchronized (lock) {
                if (!host.isWritableLocked() || !writerRunning) {
                    writeQueue.remove(chunk);
                    return -1;
                }
            }
        } catch (final InterruptedException e) {
            Thread.currentThread().interrupt();
            return -1;
        }
        return length;
    }

    private void runWriteLoop(final String address) {
        while (writerRunning) {
            final byte[] chunk;
            try {
                chunk = writeQueue.take();
            } catch (final InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
            if (chunk == WRITER_STOP || !writerRunning) {
                break;
            }
            if (!writeChunkWithAcks(chunk, address)) {
                host.fireException(EXC_RESOURCE, "Write failed for " + address);
                break;
            }
        }
    }

    /** Splits a queued chunk into device-sized sub-writes, acking each to C++ so accounting never desyncs on a partial write.
     *  Returns false only on a genuine sub-write failure; a concurrent close returns true (the close path zeroes the count). */
    private boolean writeChunkWithAcks(final byte[] chunk, final String address) {
        final int length = chunk.length;
        int offset = 0;
        int timeoutRetries = 0;
        while (offset < length) {
            final int base = offset;
            final long handle;
            int written;
            final int subLen;
            final byte[] subBuf;
            final UsbSerialPort p;
            synchronized (lock) {
                if (!host.isWritableLocked() || !writerRunning) {
                    return true;
                }
                handle = host.nativeHandleLocked();
                subLen = Math.min(host.writeChunkSizeLocked(), length - base);
                p = host.openPortOrWarnLocked("write");
                if (p == null) {
                    return false;
                }
                // Copy the exact sub-range (mik3y writes from index 0): never mutate chunk in place — a partial write advances base, so an in-place slide would clobber bytes a later sub-write still needs.
                subBuf = (base == 0 && subLen == length) ? chunk : Arrays.copyOfRange(chunk, base, base + subLen);
            }
            // Blocking I/O runs outside the lock: holding it across a wedged write starves teardown (all paths need the lock) and made cancelPendingWrites unreachable, crashing the worker on unplug. Concurrent close is safe — CDC returns on a closed connection, FTDI is interrupted by cancelPendingWrites.
            try {
                p.write(subBuf, subLen, WRITER_WRITE_TIMEOUT_MS);
                written = subLen;
            } catch (final SerialTimeoutException e) {
                // Transient stall (e.g. XOFF), thrown distinct from a lost-connection IOException so we don't tear the link down.
                // Ack exactly bytesTransferred (never subLen) and advance, so the retry re-sends only the remainder and C++ inFlightBytes never desyncs.
                written = Math.max(0, e.bytesTransferred);
                host.ackBytesWritten(handle, written);
                offset += written;
                if (++timeoutRetries > WRITER_WRITE_MAX_TIMEOUT_RETRIES) {
                    QGCLogger.w(TAG, "Write stalled past " + WRITER_WRITE_MAX_TIMEOUT_RETRIES
                            + " timeouts on " + address + "; treating as failure");
                    return false;
                }
                continue;
            } catch (final IOException e) {
                // Lost-connection failure (incl. D2XX short-write): accepted bytes are unrecoverable, but the close after fireException zeroes C++ inFlightBytes so accounting reconciles.
                QGCLogger.e(TAG, "Write failed on " + address, e);
                return false;
            }
            if (written <= 0) {
                return false;
            }
            timeoutRetries = 0;
            host.ackBytesWritten(handle, written);
            offset += written;
        }
        return true;
    }
}
