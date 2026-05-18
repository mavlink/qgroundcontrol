package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

public class AsyncUsbWritePumpTest {

    private AsyncUsbWritePump pump;

    @After
    public void tearDown() {
        if (pump != null) pump.close();
    }

    /** A WriteOp that records each invocation and counts bytes. */
    private static final class RecordingWriteOp implements AsyncUsbWritePump.WriteOp {
        final AtomicInteger calls = new AtomicInteger();
        final AtomicInteger bytes = new AtomicInteger();
        final CountDownLatch firstCall = new CountDownLatch(1);
        final AtomicReference<byte[]> lastBuf = new AtomicReference<>();
        @Override public int write(final byte[] buf, final int length, final int timeoutMs) {
            calls.incrementAndGet();
            bytes.addAndGet(length);
            lastBuf.set(buf);
            firstCall.countDown();
            return length;
        }
    }

    /** Counts onComplete invocations + records the buffer to verify ownership returns. */
    private static final class ReleaseTracker implements Runnable {
        final AtomicInteger calls = new AtomicInteger();
        @Override public void run() { calls.incrementAndGet(); }
    }

    @Test
    public void submit_reachesWorker_andFiresOnComplete() throws InterruptedException {
        final RecordingWriteOp op = new RecordingWriteOp();
        pump = AsyncUsbWritePump.forD2xx(op);
        final ReleaseTracker release = new ReleaseTracker();

        final byte[] payload = new byte[64];
        final int rc = pump.submit(payload, payload.length, release);

        assertEquals(64, rc);
        assertTrue("worker did not run", op.firstCall.await(2, TimeUnit.SECONDS));
        assertEquals(64, op.bytes.get());
        // Pump must enqueue the SAME buffer the caller handed in — no internal copy.
        assertSame(payload, op.lastBuf.get());
        // Worker must fire the release callback after the write completes.
        for (int i = 0; i < 50 && release.calls.get() == 0; i++) Thread.sleep(10);
        assertEquals(1, release.calls.get());
    }

    @Test
    public void submit_invalidLength_returnsMinusOne_andFiresOnComplete() {
        pump = AsyncUsbWritePump.forD2xx(new RecordingWriteOp());
        final ReleaseTracker release = new ReleaseTracker();

        assertEquals(-1, pump.submit(new byte[1], 0, release));
        assertEquals(-1, pump.submit(new byte[1], -5, release));
        assertEquals(-1, pump.submit(new byte[32 * 1024], 32 * 1024, release));  // > MAX_SUBMIT_BYTES
        assertEquals(-1, pump.submit(null, 8, release));
        assertEquals(-1, pump.submit(new byte[4], 8, release));  // length > data.length

        // All five failure paths must release — otherwise callers leak pool buffers.
        assertEquals(5, release.calls.get());
    }

    @Test
    public void submit_afterClose_returnsMinusOne_andFiresOnComplete() {
        pump = AsyncUsbWritePump.forD2xx(new RecordingWriteOp());
        pump.close();
        final ReleaseTracker release = new ReleaseTracker();

        assertEquals(-1, pump.submit(new byte[8], 8, release));
        assertEquals(1, release.calls.get());
    }

    @Test
    public void submit_throttle_starvation_returnsMinusOneOnDeadline() {
        // Worker never returns from write — the queue fills and submit() eventually
        // returns -1 via SUBMIT_TIMEOUT_MS without the test hanging.
        // SUBMIT_TIMEOUT_MS is 2000ms; we cap at 3500ms to give scheduler slack.
        final AsyncUsbWritePump.WriteOp blockingOp = (buf, len, t) -> {
            try { Thread.sleep(60_000); } catch (final InterruptedException e) { Thread.currentThread().interrupt(); }
            return len;
        };
        pump = AsyncUsbWritePump.forD2xx(blockingOp);
        final ReleaseTracker release = new ReleaseTracker();

        // baud = 100 → 10 B/s refill → 16 KiB submit needs >100 min; deadline (2s) trips first.
        pump.setBaudRate(100);

        final long start = System.nanoTime();
        final int rc = pump.submit(new byte[16 * 1024 - 1], 16 * 1024 - 1, release);
        final long elapsedMs = (System.nanoTime() - start) / 1_000_000L;

        assertEquals(-1, rc);
        assertEquals("throttle-timeout path must still fire onComplete", 1, release.calls.get());
        assertTrue("submit returned before 1500ms (elapsed=" + elapsedMs + ")", elapsedMs >= 1500);
        assertTrue("submit hung past 3500ms (elapsed=" + elapsedMs + ")", elapsedMs <= 3500);
    }

    @Test
    public void close_unblocksThrottledSubmit_andFiresOnComplete() throws InterruptedException {
        // Worker accepts writes instantly so the queue never blocks; the only path that
        // can stall submit() is drainTokens(). close() must wake the bucketLock.wait().
        pump = AsyncUsbWritePump.forD2xx(new RecordingWriteOp());
        pump.setBaudRate(50);  // 5 B/s — drainTokens for any reasonable payload will park
        final ReleaseTracker release = new ReleaseTracker();

        final CountDownLatch entered = new CountDownLatch(1);
        final AtomicInteger result = new AtomicInteger(Integer.MIN_VALUE);
        final Thread t = new Thread(() -> {
            entered.countDown();
            result.set(pump.submit(new byte[4096], 4096, release));
        }, "submit-under-throttle");
        t.setDaemon(true);
        t.start();

        assertTrue(entered.await(1, TimeUnit.SECONDS));
        Thread.sleep(50);
        pump.close();

        t.join(2_000);
        assertNotEquals("submit thread still stuck after close", Integer.MIN_VALUE, result.get());
        assertEquals(-1, result.get());
        assertEquals("close-unblock path must fire onComplete", 1, release.calls.get());
    }

    @Test
    public void close_drainsQueuedTasks_andFiresAllOnComplete() throws InterruptedException {
        // Worker blocks on its first call so subsequent submits queue up; close() must
        // then drain the unprocessed queue and fire each task's onComplete. Without this
        // drain step the caller's buffer pool slowly bleeds.
        final CountDownLatch workerEntered = new CountDownLatch(1);
        final CountDownLatch workerHold = new CountDownLatch(1);
        final AsyncUsbWritePump.WriteOp blockingOp = (buf, len, t) -> {
            workerEntered.countDown();
            try { workerHold.await(); } catch (final InterruptedException e) { Thread.currentThread().interrupt(); }
            return len;
        };
        pump = AsyncUsbWritePump.forD2xx(blockingOp);
        final ReleaseTracker release = new ReleaseTracker();

        // First submit enters the worker and blocks; subsequent queue up.
        // D2XX_WORKER_THREADS = 1, queue capacity = workerCount = 1.
        // So after the worker takes 1 task, the queue holds 1 more before submit blocks.
        pump.submit(new byte[8], 8, release);
        assertTrue(workerEntered.await(1, TimeUnit.SECONDS));
        pump.submit(new byte[8], 8, release);  // queued, not yet processed

        // close() while one task is in-flight in the worker (its onComplete still fires
        // via the worker's finally) and one is queued (must be drained-and-released).
        workerHold.countDown();  // let the worker complete its current task before close interrupts
        pump.close();

        // Both submits → both onCompletes (one via worker finally, one via close drain).
        assertEquals(2, release.calls.get());
    }

    @Test
    public void setBaudRate_zero_disablesThrottle() throws InterruptedException {
        final RecordingWriteOp op = new RecordingWriteOp();
        pump = AsyncUsbWritePump.forD2xx(op);

        pump.setBaudRate(100);
        pump.setBaudRate(0);
        final ReleaseTracker release = new ReleaseTracker();
        assertEquals(128, pump.submit(new byte[128], 128, release));
        assertTrue(op.firstCall.await(2, TimeUnit.SECONDS));
    }
}
