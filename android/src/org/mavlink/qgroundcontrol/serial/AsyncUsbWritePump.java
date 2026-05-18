package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Async write pump that decouples the JNI write thread from the actual USB transfer.
 * The caller's submit() enqueues a buffer and returns immediately (modulo backpressure
 * + optional wire-rate throttling); N worker threads drain the queue via a pluggable
 * {@link WriteOp}.
 *
 * <p>Two transfer strategies are supported via factory methods:</p>
 * <ul>
 *   <li>{@link #forBulkTransfer}: N=4 workers calling {@link UsbDeviceConnection#bulkTransfer}.
 *       Linux usbfs queues multiple URBs per endpoint when submitted from different threads,
 *       so this pipelines without touching {@link android.hardware.usb.UsbRequest} — and
 *       therefore without racing mik3y's read path on the shared per-connection waiter.
 *       Used for CDC-ACM, CP210x, CH34x, and mik3y-mode FTDI.</li>
 *   <li>{@link #forD2xx}: N=1 worker calling D2XX's {@code FT_Device.write(wait=true)}.
 *       D2XX claims the FTDI interface exclusively on its own internal connection, so
 *       parallel-connection bulkTransfer fails kernel-side; the only viable D2XX write
 *       path is D2XX's own API, which is single-URB and must be serialized. The worker
 *       blocks at wire rate per call; the JNI thread is shielded by the queue + token
 *       bucket. Used for QGCFtdiSerialDriver (D2XX-backed) FTDI ports.</li>
 * </ul>
 *
 * <p>Threading: caller invokes {@link #submit} from the JNI write thread (single-producer);
 * worker threads pull from a bounded queue. submit() blocks on queue full up to
 * SUBMIT_TIMEOUT_MS — that's the backpressure mechanism — with the same hard cap defending
 * the owner thread against pump stalls. When {@link #setBaudRate} is non-zero, submit() also
 * blocks on a token bucket refilling at baud/10 bytes/sec — required for D2XX FTDI ports
 * because the chip's 256 B hardware TX FIFO overruns silently above wire rate.</p>
 */
final class AsyncUsbWritePump {

    private static final String TAG = AsyncUsbWritePump.class.getSimpleName();

    private static final int BULK_WORKER_THREADS = 4;
    private static final int D2XX_WORKER_THREADS = 1;
    /** Hard cap for a single submit; matches C++ {@code MAX_SYNC_WRITE_CHUNK}. */
    private static final int MAX_SUBMIT_BYTES = 16 * 1024;
    /** Per-write kernel/D2XX timeout. Long enough that a healthy cube never hits it. */
    private static final int WRITE_TIMEOUT_MS = 1000;
    /** Max time submit() blocks for queue space before returning -1; defends owner thread against pump stalls. */
    private static final long SUBMIT_TIMEOUT_MS = 2000L;
    /** Max time to wait for worker threads to drain on close. */
    private static final long CLOSE_JOIN_MS = 500L;
    /** Token bucket capacity in bytes; ~FT232R TX FIFO (256 B) + completion-latency margin. */
    private static final long BUCKET_CAPACITY_BYTES = 512L;

    /** Pluggable write strategy. Returns bytes written, or negative on failure. */
    @FunctionalInterface
    interface WriteOp {
        int write(byte[] buf, int length, int timeoutMs) throws Exception;
    }

    /** Queue item carrying caller-owned buffer + a release callback the worker fires
     *  once the write completes (success or failure). Keeps the pump zero-allocation
     *  on the hot path — the byte[] lives in the caller's pool, not in pump-owned
     *  scratch space. */
    private static final class WriteTask {
        final byte[] buf;
        final int length;
        final Runnable onComplete;
        WriteTask(final byte[] buf, final int length, final Runnable onComplete) {
            this.buf = buf;
            this.length = length;
            this.onComplete = onComplete;
        }
    }

    private final WriteOp writeOp;
    private final BlockingQueue<WriteTask> queue;
    private final Thread[] workers;
    private final AtomicBoolean closed = new AtomicBoolean(false);

    private volatile int wireBaudBitsPerSec = 0;
    private final Object bucketLock = new Object();
    private long bucketTokensBytes;
    private long bucketLastRefillNanos;

    /** Pump using bulkTransfer with 4 parallel workers. For devices whose interface is owned
     *  by our {@link UsbDeviceConnection} (CDC-ACM, CP210x, CH34x, mik3y FTDI). */
    static AsyncUsbWritePump forBulkTransfer(final UsbDeviceConnection connection,
                                             final UsbEndpoint writeEndpoint) {
        return new AsyncUsbWritePump(
            (buf, len, timeoutMs) -> connection.bulkTransfer(writeEndpoint, buf, len, timeoutMs),
            BULK_WORKER_THREADS, "bulk");
    }

    /** Pump using a D2XX write op with a single worker. The op must call
     *  {@code FT_Device.write(buf, len, true, timeoutMs)} on D2XX's own connection — only
     *  viable D2XX write path. wait=true blocks the worker at wire rate. */
    static AsyncUsbWritePump forD2xx(final WriteOp d2xxWriteOp) {
        return new AsyncUsbWritePump(d2xxWriteOp, D2XX_WORKER_THREADS, "d2xx");
    }

    private AsyncUsbWritePump(final WriteOp writeOp, final int workerCount, final String mode) {
        this.writeOp = writeOp;
        this.queue = new LinkedBlockingQueue<>(workerCount);
        this.workers = new Thread[workerCount];
        for (int i = 0; i < workerCount; i++) {
            final Thread t = new Thread(this::runWorker, "QGCAsyncUsbWritePump-" + mode + "-" + i);
            t.setDaemon(true);
            workers[i] = t;
            t.start();
        }
    }

    /** Enables wire-rate throttling. baud=0 disables. Call after setParameters succeeds. */
    void setBaudRate(final int baud) {
        synchronized (bucketLock) {
            wireBaudBitsPerSec = baud;
            bucketTokensBytes = BUCKET_CAPACITY_BYTES;
            bucketLastRefillNanos = System.nanoTime();
            bucketLock.notifyAll();
        }
    }

    /** Ownership-transfer submit. Pump takes ownership of {@code data} until the worker
     *  fires {@code onComplete}; the caller MUST NOT touch the buffer between submit
     *  returning and onComplete running. onComplete is invoked exactly once on every
     *  code path — including all -1 returns — so callers can safely use it for pool
     *  release / refcount decrement. The byte[] is enqueued as-is: no copy.
     *  <p>Returns {@code length} on successful enqueue, -1 on closed / invalid /
     *  submit timeout / throttle starvation. Blocks until the queue has space (and
     *  tokens, if throttled).</p> */
    int submit(final byte[] data, final int length, final Runnable onComplete) {
        if (closed.get()) {
            onComplete.run();
            return -1;
        }
        if (length <= 0 || length > MAX_SUBMIT_BYTES || data == null || data.length < length) {
            QGCLogger.w(TAG, "submit invalid length " + length);
            onComplete.run();
            return -1;
        }
        if (wireBaudBitsPerSec > 0 && !drainTokens(length)) {
            QGCLogger.w(TAG, "submit throttle timeout (len=" + length + ", baud=" + wireBaudBitsPerSec + ")");
            onComplete.run();
            return -1;
        }
        final boolean offered;
        try {
            offered = queue.offer(new WriteTask(data, length, onComplete),
                                  SUBMIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        } catch (final InterruptedException e) {
            Thread.currentThread().interrupt();
            onComplete.run();
            return -1;
        }
        if (!offered) {
            QGCLogger.w(TAG, "submit timeout — pump queue full");
            onComplete.run();
            return -1;
        }
        return length;
    }

    /** Token-bucket gate. Refill = baud/10 bytes/sec (8N1). */
    private boolean drainTokens(final int bytes) {
        final long deadlineNanos = System.nanoTime() + SUBMIT_TIMEOUT_MS * 1_000_000L;
        synchronized (bucketLock) {
            while (true) {
                final long now = System.nanoTime();
                final int baud = wireBaudBitsPerSec;
                if (baud <= 0) return true;
                final long elapsed = now - bucketLastRefillNanos;
                if (elapsed > 0) {
                    final long refilled = elapsed * baud / 10L / 1_000_000_000L;
                    if (refilled > 0) {
                        bucketTokensBytes = Math.min(BUCKET_CAPACITY_BYTES, bucketTokensBytes + refilled);
                        bucketLastRefillNanos = now;
                    }
                }
                if (bucketTokensBytes >= bytes) {
                    bucketTokensBytes -= bytes;
                    return true;
                }
                if (now >= deadlineNanos) return false;
                final long needBytes = bytes - bucketTokensBytes;
                long waitNanos = needBytes * 10L * 1_000_000_000L / baud;
                if (waitNanos > deadlineNanos - now) waitNanos = deadlineNanos - now;
                if (waitNanos < 1_000_000L) waitNanos = 1_000_000L;
                try {
                    bucketLock.wait(waitNanos / 1_000_000L, (int) (waitNanos % 1_000_000L));
                } catch (final InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return false;
                }
                if (closed.get()) return false;
            }
        }
    }

    private void runWorker() {
        while (!closed.get()) {
            final WriteTask task;
            try {
                task = queue.poll(250, TimeUnit.MILLISECONDS);
            } catch (final InterruptedException e) {
                Thread.currentThread().interrupt();
                return;
            }
            if (task == null) continue;
            try {
                final int n = writeOp.write(task.buf, task.length, WRITE_TIMEOUT_MS);
                if (n < 0 && !closed.get()) {
                    QGCLogger.w(TAG, "write failed (n=" + n + ", len=" + task.length + ")");
                }
            } catch (final Throwable t) {
                if (!closed.get()) {
                    QGCLogger.w(TAG, "write threw: " + t);
                }
            } finally {
                // Always release — the buffer is caller-owned and must return to its pool /
                // get decref'd whether the write succeeded, failed, or threw.
                try { task.onComplete.run(); }
                catch (final Throwable t) { QGCLogger.w(TAG, "onComplete threw: " + t); }
            }
        }
    }

    void close() {
        if (!closed.compareAndSet(false, true)) return;
        synchronized (bucketLock) { bucketLock.notifyAll(); }
        for (final Thread t : workers) t.interrupt();
        for (final Thread t : workers) {
            try {
                t.join(CLOSE_JOIN_MS);
            } catch (final InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        // Drain any tasks the workers didn't pull before exiting and fire their
        // onComplete so caller-owned buffers go back to their pools. Without this
        // the pool slowly leaks on every close().
        for (WriteTask task; (task = queue.poll()) != null; ) {
            try { task.onComplete.run(); }
            catch (final Throwable t) { QGCLogger.w(TAG, "onComplete threw on close: " + t); }
        }
    }
}
