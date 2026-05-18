package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import java.util.concurrent.atomic.AtomicLong;

/**
 * Lightweight rolling-window throughput counter used to benchmark JNI write/read
 * crossings. Records call count + byte total per window and emits a single INFO
 * log line every {@link #WINDOW_NS} nanoseconds.
 *
 * <p>Lock-free: callers contend only on two {@link AtomicLong}s on the fast path.
 * The periodic flush is gated by a CAS on {@code windowStartNs} so exactly one
 * thread snapshots the counters per window.</p>
 */
final class RateMonitor {

    private static final long WINDOW_NS = 2_000_000_000L;

    private final String tag;
    private final String label;
    private final AtomicLong calls = new AtomicLong();
    private final AtomicLong bytes = new AtomicLong();
    private final AtomicLong maxLen = new AtomicLong();
    private final AtomicLong windowStartNs;

    RateMonitor(final String tag, final String label) {
        this.tag = tag;
        this.label = label;
        this.windowStartNs = new AtomicLong(System.nanoTime());
    }

    void record(final int n) {
        calls.incrementAndGet();
        bytes.addAndGet(n);
        // Atomic max — only update if larger.
        long prevMax;
        do {
            prevMax = maxLen.get();
            if (n <= prevMax) break;
        } while (!maxLen.compareAndSet(prevMax, n));

        final long now = System.nanoTime();
        final long start = windowStartNs.get();
        if (now - start < WINDOW_NS) return;
        if (!windowStartNs.compareAndSet(start, now)) return;

        final long c = calls.getAndSet(0);
        final long b = bytes.getAndSet(0);
        final long mx = maxLen.getAndSet(0);
        if (c == 0) return;
        final double secs = (now - start) / 1e9;
        QGCLogger.i(tag, String.format(
                "bench %s calls=%d bytes=%d avg=%.1f max=%d rate=%.1f/s bps=%.0f window=%.2fs",
                label, c, b, (double) b / c, mx, c / secs, b / secs, secs));
    }
}
