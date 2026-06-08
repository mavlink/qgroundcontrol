package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.hoho.android.usbserial.driver.UsbSerialPort;

import org.junit.After;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

public class SerialWriteLoopTest {

    private static final String ADDR = "dev/0/0";

    private SerialWriteLoop loop;

    @After
    public void tearDown() {
        if (loop != null) {
            loop.stopUnlocked(ADDR);
        }
    }

    private static final class FakeWriteHost implements SerialWriteLoop.WriteHost {
        final Object lock = new Object();
        volatile boolean writable = true;
        volatile UsbSerialPort port;
        volatile int chunkSize = SerialWireConstants.MAX_CHUNK_BYTES;
        final long handle = 0x99L;

        final AtomicLong totalAcked = new AtomicLong();
        final List<Integer> ackSizes = Collections.synchronizedList(new ArrayList<>());
        final AtomicInteger exceptionCalls = new AtomicInteger();
        final AtomicInteger lastExceptionKind = new AtomicInteger(Integer.MIN_VALUE);
        final CountDownLatch exceptionLatch = new CountDownLatch(1);

        @Override public Object lock() { return lock; }
        @Override public boolean isWritableLocked() { return writable; }
        @Override public long nativeHandleLocked() { return handle; }
        @Override public int writeChunkSizeLocked() { return chunkSize; }
        @Override public UsbSerialPort openPortOrWarnLocked(final String operation) { return port; }
        @Override public void muteListenerLocked() { }
        @Override public void cancelPendingWritesUnlocked() { }

        @Override public void ackBytesWritten(final long h, final int n) {
            totalAcked.addAndGet(n);
            ackSizes.add(n);
        }

        @Override public void fireException(final int kind, final String message) {
            lastExceptionKind.set(kind);
            exceptionCalls.incrementAndGet();
            exceptionLatch.countDown();
        }
    }

    private static ByteBuffer payload(final int length) {
        return ByteBuffer.allocate(length);
    }

    private boolean startLocked(final SerialWriteLoop l, final FakeWriteHost host) {
        synchronized (host.lock) {
            return l.startLocked(ADDR);
        }
    }

    @Test
    public void startLocked_refusesWhenLeakedWriterStillAlive() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        final CountDownLatch blockWrite = new CountDownLatch(1);
        final CountDownLatch writeEntered = new CountDownLatch(1);
        host.port = new FakeUsbSerialPort((data, length, call) -> {
            writeEntered.countDown();
            try {
                blockWrite.await();
            } catch (final InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
        });

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));
        assertEquals(256, loop.enqueue(payload(256), 256, ADDR));
        assertTrue(writeEntered.await(2, TimeUnit.SECONDS));

        loop.stopUnlocked(ADDR);

        synchronized (host.lock) {
            assertFalse(loop.startLocked(ADDR));
        }
        assertFalse(loop.isRunning());

        blockWrite.countDown();
    }

    @Test
    public void startLocked_clearsSelfExitedWriterAndStartsFresh() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));

        host.writable = false;
        loop.stopUnlocked(ADDR);
        assertFalse(loop.isRunning());

        host.writable = true;
        synchronized (host.lock) {
            assertTrue(loop.startLocked(ADDR));
        }
        assertTrue(loop.isRunning());
    }

    @Test
    public void startLocked_returnsTrueEarlyWhenWriterAlreadyAlive() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));
        assertTrue(loop.isRunning());

        synchronized (host.lock) {
            assertTrue(loop.startLocked(ADDR));
        }
        assertTrue(loop.isRunning());
    }

    @Test
    public void stopUnlocked_parksThreadAndBlocksRestartWhenJoinTimesOut() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        final CountDownLatch blockWrite = new CountDownLatch(1);
        final CountDownLatch writeEntered = new CountDownLatch(1);
        host.port = new FakeUsbSerialPort((data, length, call) -> {
            writeEntered.countDown();
            try {
                blockWrite.await();
            } catch (final InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
        });

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));
        assertEquals(256, loop.enqueue(payload(256), 256, ADDR));
        assertTrue(writeEntered.await(2, TimeUnit.SECONDS));

        loop.stopUnlocked(ADDR);

        synchronized (host.lock) {
            assertFalse(loop.startLocked(ADDR));
        }

        blockWrite.countDown();
    }

    @Test
    public void timeoutRetryExhaustion_firesResourceAndAcksTransferredBytes() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.alwaysTimeout(10));

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));
        assertEquals(256, loop.enqueue(payload(256), 256, ADDR));

        assertTrue(host.exceptionLatch.await(5, TimeUnit.SECONDS));
        assertEquals(1, host.exceptionCalls.get());
        assertEquals(SerialWireConstants.EXC_RESOURCE, host.lastExceptionKind.get());
        assertEquals(List.of(10, 10, 10), host.ackSizes);
        assertEquals(30L, host.totalAcked.get());
    }

    @Test
    public void concurrentCloseDuringSubWriteLoop_returnsCleanlyWithoutException() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.chunkSize = 64;
        final AtomicBoolean flipped = new AtomicBoolean();
        final CountDownLatch firstWrite = new CountDownLatch(1);
        host.port = new FakeUsbSerialPort((data, length, call) -> {
            if (flipped.compareAndSet(false, true)) {
                host.writable = false;
                firstWrite.countDown();
            }
        });

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));
        assertEquals(256, loop.enqueue(payload(256), 256, ADDR));

        assertTrue(firstWrite.await(5, TimeUnit.SECONDS));
        loop.stopUnlocked(ADDR);
        assertFalse(loop.isRunning());
        assertEquals(0, host.exceptionCalls.get());
    }

    @Test
    public void enqueue_rejectsInvalidRequestWithSentinel() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));

        assertEquals(-1, loop.enqueue(null, 256, ADDR));
        assertEquals(-1, loop.enqueue(payload(256), 0, ADDR));
        assertEquals(-1, loop.enqueue(payload(256), -5, ADDR));
    }

    @Test
    public void enqueue_rejectsBeforeStartWithSentinel() {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        loop = new SerialWriteLoop(host);

        assertFalse(loop.isRunning());
        assertEquals(-1, loop.enqueue(payload(256), 256, ADDR));
    }

    @Test
    public void enqueue_rejectsWhenNotWritableWithSentinel() throws Exception {
        final FakeWriteHost host = new FakeWriteHost();
        host.port = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        loop = new SerialWriteLoop(host);
        assertTrue(startLocked(loop, host));

        host.writable = false;
        assertEquals(-1, loop.enqueue(payload(256), 256, ADDR));
    }
}
