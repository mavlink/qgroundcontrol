package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.awaitTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.port;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.portOnAnIdleLine;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.readThread;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.writeThread;

import android.hardware.usb.FakeUsbDevice;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.After;
import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.FakeUsbHost;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.RecordingCallbacks;

public class QGCUsbSerialPortTest {
    // The handle of every port here, which the callbacks report as their token.
    private static final long HANDLE = 7;
    // How long a failed transfer waits for its device to detach in the tests that sit the wait out.
    private static final long SHORT_DETACH_WAIT_MS = 50;
    // A port's I/O threads are named after its ID, so each port gets its own: writeThread() must not find the
    // thread of an earlier test's port.
    private static final AtomicInteger sNextPortId = new AtomicInteger(1);

    private final List<QGCUsbSerialPort> _ports = new ArrayList<>();

    @After
    public void tearDown() {
        // A test that fails may leave its port open, with the port's I/O threads running.
        for (final QGCUsbSerialPort port : _ports) {
            port.close();
        }
    }

    @Test
    public void ioCallbacks_reachCppUntilThePortIsClosed() {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(port(Map.of("isOpen", args -> true, "close", args -> null)), callbacks);

        port.inputArrived(new byte[] {1, 2}, 2);
        port.readFailed(new IOException("gone"));
        assertEquals(List.of("data 7 2", "resourceError 7 2 [gone]"), callbacks.events);

        assertTrue(port.close());
        port.inputArrived(new byte[] {3}, 1);
        port.readFailed(new IOException("late"));
        assertEquals(2, callbacks.events.size());
    }

    @Test
    public void closeDisconnected_afterTheReadThreadReportedIt_addsNoSecondResourceError() {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(port(Map.of("isOpen", args -> true, "close", args -> null)), callbacks);

        port.readFailed(new IOException("gone"));
        port.closeDisconnected();

        assertEquals(List.of("resourceError 7 2 [gone]"), callbacks.events);
    }

    @Test
    public void closeDisconnected_afterReadingWasRestarted_reportsAResourceErrorAgain() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portOnAnIdleLine(readsToEnd), callbacks);

        // A read failed, C++ started reading again on its next read, and only later was the device unplugged.
        port.readFailed(new IOException("glitch"));
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        port.closeDisconnected();
        readsToEnd.release(100);

        assertEquals(List.of("resourceError 7 2 [glitch]", "resourceError 7 1 []"), callbacks.events);
    }

    @Test
    public void resourceError_ofAnExceptionWithoutAMessage_namesTheException() throws Exception {
        final RecordingCallbacks readCallbacks = new RecordingCallbacks();
        final QGCUsbSerialPort readPort =
                newPort(port(Map.of("isOpen", args -> true, "close", args -> null)), readCallbacks);
        final RecordingCallbacks writeCallbacks = new RecordingCallbacks();
        final QGCUsbSerialPort writePort =
                newPort(port(Map.of("isOpen", args -> true, "close", args -> null, "write", args -> {
                    throw new IllegalStateException();
                })), writeCallbacks);

        readPort.readFailed(new IOException());
        assertEquals(1, writePort.writeAsync(new byte[1], 1));
        awaitTrue(() -> !writeCallbacks.events.isEmpty());

        // C++ logs the detail, so it gets one that is never null.
        assertEquals(List.of("resourceError 7 2 [IOException]"), readCallbacks.events);
        assertEquals(List.of("resourceError 7 3 [IllegalStateException]"), writeCallbacks.events);
    }

    @Test
    public void stopReading_dropsInputUntilReadingIsStartedAgain() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portOnAnIdleLine(readsToEnd), callbacks);

        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        assertTrue(port.isReading());

        port.stopReading();
        assertFalse(port.isReading());
        port.inputArrived(new byte[] {1}, 1);

        // The read thread is still waiting for input, which nothing can interrupt.
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        assertTrue(port.isReading());
        port.inputArrived(new byte[] {2, 3}, 2);
        assertEquals(List.of("data 7 2"), callbacks.events);

        assertTrue(port.close());
        readsToEnd.release(100);
    }

    @Test
    public void readThread_waitsWhileCppHasNoRoomAndReadsOnWhenStartedAgain() throws Exception {
        final Semaphore input = new Semaphore(0);
        final AtomicInteger reads = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portWithInput(input, reads), callbacks);
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        final Thread readThread = readThread(port);

        // C++ takes the input, but has no room for more: the device is not asked for the input that it has.
        callbacks.hasRoom = false;
        input.release(3);
        awaitTrue(() -> readThread.getState() == Thread.State.WAITING);
        assertEquals(List.of("data 7 1"), callbacks.events);
        assertEquals(1, reads.get());
        assertTrue(port.isReading());

        // C++ says that it has room again by starting to read. What the device held back follows, in order.
        callbacks.hasRoom = true;
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        awaitTrue(() -> callbacks.events.size() == 3);
        assertEquals(List.of("data 7 1", "data 7 2", "data 7 3"), callbacks.events);

        assertTrue(port.close());
        input.release(100);
    }

    @Test
    public void readThread_doesNotWaitForAStartThatCameBeforeItCouldWait() throws Exception {
        final Semaphore input = new Semaphore(0);
        final List<Integer> chunks = new CopyOnWriteArrayList<>();
        final QGCUsbSerialPort[] port = new QGCUsbSerialPort[1];
        port[0] = newPort(portWithInput(input, new AtomicInteger()), new QGCUsbSerialCallbacks() {
            @Override
            public boolean newData(final long token, final byte[] data, final int length) {
                chunks.add(length);
                if (length > 1) {
                    return true;
                }
                // C++ has no room behind the first chunk. Its own thread takes that chunk and starts reading again
                // while the read thread is still on its way back from this call.
                port[0].startReading();
                return false;
            }

            @Override
            public void bytesWritten(final long token, final int count) {}

            @Override
            public void resourceError(final long token, final int reason, final String detail) {}

            @Override
            public void devicesChanged() {}
        });
        assertEquals(QGCUsbSerialResult.OK, port[0].startReading());

        input.release(2);
        awaitTrue(() -> chunks.size() == 2);
        assertEquals(List.of(1, 2), chunks);

        assertTrue(port[0].close());
        input.release(100);
    }

    @Test
    public void close_endsAReadThreadThatWaitsForCppToHaveRoom() throws Exception {
        final Semaphore input = new Semaphore(0);
        final AtomicInteger reads = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portWithInput(input, reads), callbacks);
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        final Thread readThread = readThread(port);
        callbacks.hasRoom = false;
        input.release(3);
        awaitTrue(() -> readThread.getState() == Thread.State.WAITING);

        // No read is in flight that the closing of the driver's port could fail.
        assertTrue(port.close());
        readThread.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(readThread.isAlive());
        assertEquals(List.of("data 7 1"), callbacks.events);
        assertEquals(1, reads.get());
    }

    @Test
    public void stopReading_whileTheReadThreadWaitsForRoom_readsOnAndDropsTheInput() throws Exception {
        final Semaphore input = new Semaphore(0);
        final AtomicInteger reads = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portWithInput(input, reads), callbacks);
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        final Thread readThread = readThread(port);
        callbacks.hasRoom = false;
        input.release();
        awaitTrue(() -> readThread.getState() == Thread.State.WAITING);

        // C++ wants no more input, so the device is not left holding any: the second read is dropped.
        port.stopReading();
        assertFalse(port.isReading());
        input.release();
        awaitTrue(() -> reads.get() == 3);
        assertEquals(List.of("data 7 1"), callbacks.events);

        callbacks.hasRoom = true;
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        input.release();
        awaitTrue(() -> callbacks.events.size() == 2);
        assertEquals(List.of("data 7 1", "data 7 3"), callbacks.events);

        assertTrue(port.close());
        input.release(100);
    }

    @Test
    public void readThread_passesInputOnAndStartsAgainAfterItFailed() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final AtomicInteger reads = new AtomicInteger();
        final List<Object> readQueue = new CopyOnWriteArrayList<>();
        final List<Object> firstRead = new CopyOnWriteArrayList<>();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> {
                    readQueue.addAll(List.of(args));
                    return null;
                },
                "read", args -> {
                    final byte[] buffer = (byte[]) args[0];
                    switch (reads.incrementAndGet()) {
                        case 1:
                            firstRead.addAll(List.of(buffer.length, args[1]));
                            buffer[0] = 1;
                            buffer[1] = 2;
                            return 2;
                        case 2:
                            throw new IOException("gone");
                        default:
                            readsToEnd.tryAcquire(5, TimeUnit.SECONDS);
                            return 0;
                    }
                })), callbacks);

        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        awaitTrue(() -> callbacks.events.size() == 2);
        assertEquals(List.of("data 7 2", "resourceError 7 2 [gone]"), callbacks.events);
        assertFalse(port.isReading());
        // The driver rejects a read whose buffer is not the size of its read queue's, or that has a timeout.
        assertEquals(List.of(2, 2048), readQueue);
        assertEquals(List.of(2048, 0), firstRead);

        // C++ reads on after the error, which starts a read thread again.
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        assertTrue(port.isReading());
        awaitTrue(() -> reads.get() == 3);

        assertTrue(port.close());
        readsToEnd.release(100);
    }

    @Test
    public void startReading_afterAWriteFailed_letsAReadFailureBeReported() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> null,
                "read", args -> {
                    readsToEnd.tryAcquire(5, TimeUnit.SECONDS);
                    return 0;
                },
                "write", args -> {
                    throw new IOException("stalled");
                })), callbacks);
        assertEquals(QGCUsbSerialResult.OK, port.startReading());

        assertEquals(1, port.writeAsync(new byte[1], 1));
        awaitTrue(() -> !callbacks.events.isEmpty());
        // C++ reads on after the error, which asks to start reading though the read thread never stopped.
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        port.readFailed(new IOException("gone"));
        assertEquals(List.of("resourceError 7 3 [stalled]", "resourceError 7 2 [gone]"), callbacks.events);

        assertTrue(port.close());
        readsToEnd.release(100);
    }

    @Test
    public void writeWithProgress_closedUnderTheWrite_isALostDevice() {
        final AtomicBoolean open = new AtomicBoolean(true);
        final UsbSerialPort driverPort =
                port(Map.of("isOpen", args -> open.get(), "close", args -> null, "write", args -> {
                    // What the driver does when another thread closes the port: its connection is suddenly null.
                    open.set(false);
                    throw new NullPointerException("Connection cleared by close");
                }));
        final QGCUsbSerialPort port = newPort(driverPort, new RecordingCallbacks());

        assertEquals(QGCUsbSerialResult.IO_ERROR, port.writeWithProgress(new byte[8], 8, 100));
    }

    @Test
    public void writeAsync_writesOnItsOwnThreadAndReportsWhatItWrote() throws Exception {
        final List<String> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final UsbSerialPort driverPort = port(Map.of("isOpen", args -> true, "close", args -> null, "write", args -> {
            deviceTakesData.acquire();
            writes.add(Thread.currentThread().getName() + " " + args[1] + " timeout " + args[2]);
            return null;
        }));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(driverPort, callbacks);

        // The caller is not held up by a device that has not taken the data yet.
        assertEquals(3, port.writeAsync(new byte[] {1, 2, 3}, 3));
        assertEquals(2, port.writeAsync(new byte[] {4, 5, 6}, 2));
        assertEquals(List.of(), callbacks.events);

        deviceTakesData.release(2);
        awaitTrue(() -> callbacks.events.size() == 2);
        assertEquals(List.of("written 7 3", "written 7 2"), callbacks.events);
        final String writeThread = "QGCUsbSerialWrite-" + port.id;
        assertEquals(List.of(writeThread + " 3 timeout 0", writeThread + " 2 timeout 0"), writes);

        assertTrue(port.close());
    }

    @Test
    public void writeAsync_whenTheDeviceIsLost_reportsOneResourceErrorAndRefusesMoreOutput() throws Exception {
        final UsbSerialPort unplugged = port(Map.of("isOpen", args -> true, "close", args -> null, "write", args -> {
            throw new IOException("Error writing 1 bytes, rc=-1");
        }));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(unplugged, callbacks);

        assertEquals(1, port.writeAsync(new byte[1], 1));
        awaitTrue(() -> !callbacks.events.isEmpty());
        final List<String> lost = List.of("resourceError 7 3 [Error writing 1 bytes, rc=-1]");
        assertEquals(lost, callbacks.events);

        // C++ has been told, which it is once. Of a chunk that the port took and failed to write, it would not
        // hear.
        assertEquals(QGCUsbSerialResult.IO_ERROR, port.writeAsync(new byte[1], 1));
        assertEquals("The port has reported that it can no longer transfer data",
                     QGCUsbSerialLastError.get());
        assertEquals(lost, callbacks.events);
    }

    @Test
    public void writeAsync_closedUnderTheWrite_reportsNothing() throws Exception {
        final Semaphore closed = new Semaphore(0);
        final CountDownLatch writing = new CountDownLatch(1);
        final UsbSerialPort driverPort = port(Map.of("isOpen", args -> true, "close", args -> {
            closed.release();
            return null;
        }, "write", args -> {
            writing.countDown();
            // A transfer in flight ends when the connection closes, and not when its thread is interrupted.
            closed.acquireUninterruptibly();
            // What the driver does when another thread closes the port: its connection is suddenly null.
            throw new NullPointerException("Connection cleared by close");
        }));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(driverPort, callbacks);

        assertEquals(8, port.writeAsync(new byte[8], 8));
        assertTrue(writing.await(5, TimeUnit.SECONDS));
        final Thread blockedWrite = writeThread(port);
        // Closing does not wait for the device, and ends the write.
        assertTrue(port.close());
        blockedWrite.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(blockedWrite.isAlive());
        assertEquals(List.of(), callbacks.events);
    }

    @Test
    public void close_underAWriteInFlight_sendsNoMoreOfItsChunk() throws Exception {
        final List<byte[]> writes = new CopyOnWriteArrayList<>();
        final CountDownLatch writing = new CountDownLatch(1);
        final Semaphore transferEnds = new Semaphore(0);
        // A driver port whose transfer in flight outlasts the close, and which would take more after it.
        final UsbSerialPort driverPort = port(Map.of("isOpen", args -> true, "close", args -> null, "write", args -> {
            writing.countDown();
            transferEnds.acquireUninterruptibly();
            writes.add(Arrays.copyOf((byte[]) args[0], (Integer) args[1]));
            return null;
        }));
        final QGCUsbSerialPort port = newPort(driverPort, new RecordingCallbacks());

        assertEquals(3000, port.writeAsync(chunk(3000), 3000));
        assertTrue(writing.await(5, TimeUnit.SECONDS));
        final Thread writeInFlight = writeThread(port);
        assertTrue(port.close());

        transferEnds.release(3);
        writeInFlight.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(writeInFlight.isAlive());
        assertEquals(List.of(1024), lengths(writes));
    }

    @Test
    public void writeAsync_refusesMoreThanAFewChunksAhead() throws Exception {
        final Semaphore deviceTakesData = new Semaphore(0);
        final CountDownLatch writing = new CountDownLatch(1);
        final UsbSerialPort stalled = port(Map.of("isOpen", args -> true, "close", args -> null, "write", args -> {
            writing.countDown();
            deviceTakesData.acquire();
            return null;
        }));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(stalled, callbacks);

        // One chunk is with the device, and four more wait behind it.
        assertEquals(1, port.writeAsync(new byte[1], 1));
        assertTrue(writing.await(5, TimeUnit.SECONDS));
        for (int waiting = 0; waiting < 4; ++waiting) {
            assertEquals(1, port.writeAsync(new byte[1], 1));
        }
        assertEquals(QGCUsbSerialResult.FAILED, port.writeAsync(new byte[1], 1));

        deviceTakesData.release(5);
        awaitTrue(() -> callbacks.events.size() == 5);
        assertTrue(port.close());
    }

    @Test
    public void purgeBuffers_ofOutput_stopsTheChunkBeingWritten() throws Exception {
        final List<byte[]> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portOnASlowDevice(writes, deviceTakesData), callbacks);
        final byte[] discarded = chunk(2500);

        assertEquals(2500, port.writeAsync(discarded, 2500));
        awaitTrue(() -> writes.size() == 1);
        port.purgeBuffers(false, true);
        // The piece that the driver has cannot be taken back. The two after it are not sent, and C++ still hears
        // of every byte, which it expects of all that it handed over.
        deviceTakesData.release();
        awaitTrue(() -> !callbacks.events.isEmpty());
        assertEquals(List.of("written 7 2500"), callbacks.events);
        assertEquals(List.of(1024), lengths(writes));
        assertArrayEquals(Arrays.copyOf(discarded, 1024), writes.get(0));

        final byte[] next = chunk(1500);
        deviceTakesData.release(2);
        assertEquals(1500, port.writeAsync(next, 1500));
        awaitTrue(() -> callbacks.events.size() == 2);
        assertEquals(List.of("written 7 2500", "written 7 1500"), callbacks.events);
        assertEquals(List.of(1024, 1024, 476), lengths(writes));
        assertArrayEquals(next, joined(writes.subList(1, 3)));

        assertTrue(port.close());
    }

    @Test
    public void purgeBuffers_ofOutput_reportsTheChunksWaitingWithoutSendingThem() throws Exception {
        final List<byte[]> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portOnASlowDevice(writes, deviceTakesData), callbacks);

        assertEquals(2000, port.writeAsync(new byte[2000], 2000));
        awaitTrue(() -> writes.size() == 1);
        assertEquals(300, port.writeAsync(new byte[300], 300));
        port.purgeBuffers(false, true);
        // C++ writes on at once, while the write thread still has the discarded chunks to drop.
        assertEquals(40, port.writeAsync(new byte[40], 40));

        deviceTakesData.release(100);
        awaitTrue(() -> callbacks.events.size() == 3);
        // In the order of the chunks, by which C++ tells the bytes it discarded from those after them.
        assertEquals(List.of("written 7 2000", "written 7 300", "written 7 40"), callbacks.events);
        assertEquals(List.of(1024, 40), lengths(writes));

        assertTrue(port.close());
    }

    @Test
    public void purgeBuffers_ofInputOnly_leavesTheOutputToBeSent() throws Exception {
        final List<byte[]> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(portOnASlowDevice(writes, deviceTakesData), callbacks);
        final byte[] chunk = chunk(2500);

        assertEquals(2500, port.writeAsync(chunk, 2500));
        awaitTrue(() -> writes.size() == 1);
        port.purgeBuffers(true, false);

        deviceTakesData.release(100);
        awaitTrue(() -> !callbacks.events.isEmpty());
        assertEquals(List.of("written 7 2500"), callbacks.events);
        // In pieces, between which a discard of output would have stopped it.
        assertEquals(List.of(1024, 1024, 452), lengths(writes));
        assertArrayEquals(chunk, joined(writes));

        assertTrue(port.close());
    }

    @Test
    public void readFailure_ofADeviceThatIsGone_reportsTheDetachOnce() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    throw new IOException("Queueing USB request failed");
                })), callbacks);
        port.setDetachWaitForTesting(TimeUnit.SECONDS.toMillis(30));
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        final Thread readThread = readThread(port);
        // The read fails before Android reports the detach, and the read thread waits for that report.
        awaitTrue(() -> readThread.getState() == Thread.State.TIMED_WAITING);
        assertEquals(List.of(), callbacks.events);

        // Android's report follows, on which the devices close the port.
        port.closeDisconnected();
        readThread.join(TimeUnit.SECONDS.toMillis(5));

        assertFalse(readThread.isAlive());
        assertEquals(List.of("resourceError 7 1 []"), callbacks.events);
    }

    @Test
    public void readFailure_ofADeviceThatStaysAttached_isReportedAfterTheWaitForADetach() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    throw new IOException("glitch");
                })), callbacks);
        port.setDetachWaitForTesting(SHORT_DETACH_WAIT_MS);

        final long start = System.nanoTime();
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        awaitTrue(() -> !callbacks.events.isEmpty());
        final long reportedAfterMs = TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - start);

        assertEquals(List.of("resourceError 7 2 [glitch]"), callbacks.events);
        assertTrue("Reported after " + reportedAfterMs + " ms", reportedAfterMs >= SHORT_DETACH_WAIT_MS);
    }

    @Test
    public void close_duringTheWaitOfAFailedRead_endsTheWaitWithoutAReport() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    throw new IOException("glitch");
                })), callbacks);
        port.setDetachWaitForTesting(TimeUnit.SECONDS.toMillis(30));
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        final Thread readThread = readThread(port);
        awaitTrue(() -> readThread.getState() == Thread.State.TIMED_WAITING);

        // C++ closes the port, so it wants to hear nothing more of it.
        assertTrue(port.close());
        readThread.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(readThread.isAlive());
        assertEquals(List.of(), callbacks.events);
    }

    @Test
    public void readThread_readsOnAfterFailedStatusChecksOfADeviceThatStaysAttached() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final AtomicInteger reads = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    switch (reads.incrementAndGet()) {
                        case 1:
                        case 2:
                        case 4:
                        case 5:
                            throw new IOException(QGCUsbSerialReader.STATUS_CHECK_FAILED);
                        case 3:
                        case 6:
                            return 2;
                        default:
                            readsToEnd.tryAcquire(5, TimeUnit.SECONDS);
                            return 0;
                    }
                })), callbacks);

        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        awaitTrue(() -> reads.get() == 7);

        // Two failures in a row, twice over: the input between them starts the count again.
        assertEquals(List.of("data 7 2", "data 7 2"), callbacks.events);
        assertTrue(port.isReading());

        assertTrue(port.close());
        readsToEnd.release(100);
    }

    @Test
    public void readThread_stopsAfterThreeFailedStatusChecksInARow() throws Exception {
        final AtomicInteger reads = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    reads.incrementAndGet();
                    throw new IOException(QGCUsbSerialReader.STATUS_CHECK_FAILED);
                })), callbacks);
        port.setDetachWaitForTesting(SHORT_DETACH_WAIT_MS);

        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        awaitTrue(() -> !callbacks.events.isEmpty());

        assertEquals(List.of("resourceError 7 2 [USB get_status request failed]"), callbacks.events);
        assertEquals(3, reads.get());
        assertFalse(port.isReading());
    }

    @Test
    public void writeFailure_ofADeviceThatIsGone_reportsTheDetachOnce() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of("write", args -> {
            throw new IOException("Error writing 1 bytes, rc=-1");
        })), callbacks);
        port.setDetachWaitForTesting(TimeUnit.SECONDS.toMillis(30));

        assertEquals(1, port.writeAsync(new byte[1], 1));
        final Thread writeThread = writeThread(port);
        awaitTrue(() -> writeThread.getState() == Thread.State.TIMED_WAITING);
        assertEquals(List.of(), callbacks.events);

        port.closeDisconnected();
        writeThread.join(TimeUnit.SECONDS.toMillis(5));

        assertFalse(writeThread.isAlive());
        assertEquals(List.of("resourceError 7 1 []"), callbacks.events);
    }

    @Test
    public void writeFailure_ofADeviceThatStaysAttached_isReportedAfterTheWaitForADetach() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of("write", args -> {
            throw new IOException("stalled");
        })), callbacks);
        port.setDetachWaitForTesting(SHORT_DETACH_WAIT_MS);

        final long start = System.nanoTime();
        assertEquals(1, port.writeAsync(new byte[1], 1));
        awaitTrue(() -> !callbacks.events.isEmpty());
        final long reportedAfterMs = TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - start);

        assertEquals(List.of("resourceError 7 3 [stalled]"), callbacks.events);
        assertTrue("Reported after " + reportedAfterMs + " ms", reportedAfterMs >= SHORT_DETACH_WAIT_MS);
    }

    @Test
    public void close_duringTheWaitOfAFailedWrite_endsTheWaitWithoutAReport() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of("write", args -> {
            throw new IOException("stalled");
        })), callbacks);
        port.setDetachWaitForTesting(TimeUnit.SECONDS.toMillis(30));
        assertEquals(1, port.writeAsync(new byte[1], 1));
        final Thread writeThread = writeThread(port);
        awaitTrue(() -> writeThread.getState() == Thread.State.TIMED_WAITING);

        assertTrue(port.close());
        writeThread.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(writeThread.isAlive());
        assertEquals(List.of(), callbacks.events);
    }

    @Test
    public void writeAsync_afterReadingStartedAgain_isAcceptedAndItsFailureReported() throws Exception {
        final Semaphore readsToEnd = new Semaphore(0);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = newPort(port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> null,
                "read", args -> {
                    readsToEnd.tryAcquire(5, TimeUnit.SECONDS);
                    return 0;
                },
                "write", args -> {
                    throw new IOException("stalled");
                })), callbacks);
        assertEquals(1, port.writeAsync(new byte[1], 1));
        awaitTrue(() -> !callbacks.events.isEmpty());
        assertEquals(QGCUsbSerialResult.IO_ERROR, port.writeAsync(new byte[1], 1));

        // C++ reads on after the error, and with that asks to be told of the next one.
        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        assertEquals(1, port.writeAsync(new byte[1], 1));
        awaitTrue(() -> callbacks.events.size() == 2);
        assertEquals(List.of("resourceError 7 3 [stalled]", "resourceError 7 3 [stalled]"), callbacks.events);

        assertTrue(port.close());
        readsToEnd.release(100);
    }

    @Test
    public void writeAsync_whileAFailedWriteWaitsToBeReported_isWrittenByAnotherThread() throws Exception {
        final AtomicInteger writes = new AtomicInteger();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialPort port = openPort(new FakeUsbHost(), port(Map.of("write", args -> {
            if (writes.incrementAndGet() == 1) {
                throw new IOException("glitch");
            }
            return null;
        })), callbacks);
        // The device stays attached, so the thread of the failed write waits for a detach before it reports.
        port.setDetachWaitForTesting(TimeUnit.SECONDS.toMillis(30));
        assertEquals(1, port.writeAsync(new byte[1], 1));
        final Thread failedWrite = writeThread(port);
        awaitTrue(() -> failedWrite.getState() == Thread.State.TIMED_WAITING);

        // C++ has heard of no failure and hands over more, as it may once it has discarded its output. The
        // thread that failed takes no more chunks.
        assertEquals(2, port.writeAsync(new byte[2], 2));
        awaitTrue(() -> !callbacks.events.isEmpty());
        assertEquals(List.of("written 7 2"), callbacks.events);

        assertTrue(port.close());
        failedWrite.join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(failedWrite.isAlive());
    }

    @Test
    public void setOutputLine_ofALineThatTheDeviceDrives_setsNoLine() {
        // No handler for DTR or RTS: setting either in place of the line asked for would fail the test.
        final QGCUsbSerialPort port = newPort(port(Map.of(
                "isOpen", args -> false,
                "getSupportedControlLines", args -> EnumSet.allOf(UsbSerialPort.ControlLine.class))),
                new RecordingCallbacks());

        assertThrows(UnsupportedOperationException.class,
                     () -> port.setOutputLine(UsbSerialPort.ControlLine.CTS, true));
    }

    @Test
    public void startReading_onAClosedPort_isNotOpen() {
        final QGCUsbSerialPort port =
                newPort(port(Map.of("isOpen", args -> true, "close", args -> null)), new RecordingCallbacks());

        assertTrue(port.close());
        assertEquals(QGCUsbSerialResult.NOT_OPEN, port.startReading());
    }

    @Test
    public void read_leavesThePortsInputToTheReadThreadOnceStarted() throws Exception {
        final Thread testThread = Thread.currentThread();
        final AtomicInteger directReads = new AtomicInteger();
        final CountDownLatch readThreadReading = new CountDownLatch(1);
        final CountDownLatch releaseReadThread = new CountDownLatch(1);
        final UsbSerialPort driverPort = port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> null,
                "read", args -> {
                    if (Thread.currentThread() == testThread) {
                        directReads.incrementAndGet();
                    } else {
                        readThreadReading.countDown();
                        releaseReadThread.await(5, TimeUnit.SECONDS);
                    }
                    return 0;
                }));
        final QGCUsbSerialPort port = newPort(driverPort, new RecordingCallbacks());

        assertEquals(0, port.read(16, 200).length);
        assertEquals(1, directReads.get());

        assertEquals(QGCUsbSerialResult.OK, port.startReading());
        assertTrue(readThreadReading.await(5, TimeUnit.SECONDS));
        assertEquals(0, port.read(16, 200).length);
        assertEquals(1, directReads.get());

        port.stopReading();
        assertEquals(0, port.read(16, 200).length);
        assertEquals(1, directReads.get());

        assertTrue(port.close());
        releaseReadThread.countDown();
    }

    private QGCUsbSerialPort newPort(final UsbSerialPort driverPort, final QGCUsbSerialCallbacks callbacks) {
        final QGCUsbSerialPort port = new QGCUsbSerialPort(sNextPortId.getAndIncrement(), HANDLE,
                new QGCUsbSerialDevices.PortAddress(1, 0), driverPort, callbacks);
        // A failed transfer is reported at once, except in the tests that are about its wait for a detach.
        port.setDetachWaitForTesting(0);
        _ports.add(port);
        return port;
    }

    /**
     * A port that is open on a device of the host, as {@link QGCUsbSerialDevices} opens one. The driver port
     * needs only the methods that the test uses on the open port.
     */
    private QGCUsbSerialPort openPort(final FakeUsbHost usbHost, final UsbSerialPort driverPort,
                                      final QGCUsbSerialCallbacks callbacks) {
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME), driverPort);
        final QGCUsbSerialPort port = newPort(usbHost.drivers.get(0).getPorts().get(0), callbacks);
        assertEquals(QGCUsbSerialResult.OK, port.open(usbHost));
        return port;
    }

    /**
     * A driver port whose device has input for a read once {@code input} has a permit for it. Each read returns a
     * byte more than the one before it, so that the length of a chunk tells which read it is from, and
     * {@code reads} counts the reads that have begun.
     */
    private static UsbSerialPort portWithInput(final Semaphore input, final AtomicInteger reads) {
        return port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> null,
                "read", args -> {
                    final int read = reads.incrementAndGet();
                    return input.tryAcquire(5, TimeUnit.SECONDS) ? read : 0;
                }));
    }

    /**
     * A driver port whose device takes a write only once {@code deviceTakesData} has a permit for it. Each write
     * is listed in {@code writes}, by its bytes, as it begins.
     */
    private static UsbSerialPort portOnASlowDevice(final List<byte[]> writes, final Semaphore deviceTakesData) {
        return port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "purgeHwBuffers", args -> null,
                "write", args -> {
                    writes.add(Arrays.copyOf((byte[]) args[0], (Integer) args[1]));
                    deviceTakesData.acquire();
                    return null;
                }));
    }

    /** Bytes that differ from one piece of a chunk to the next, so that a piece from the wrong place shows. */
    private static byte[] chunk(final int length) {
        final byte[] chunk = new byte[length];
        for (int i = 0; i < length; i++) {
            chunk[i] = (byte) (i % 251);
        }
        return chunk;
    }

    private static List<Integer> lengths(final List<byte[]> writes) {
        final List<Integer> lengths = new ArrayList<>();
        for (final byte[] write : writes) {
            lengths.add(write.length);
        }
        return lengths;
    }

    private static byte[] joined(final List<byte[]> writes) {
        final ByteArrayOutputStream joined = new ByteArrayOutputStream();
        for (final byte[] write : writes) {
            joined.write(write, 0, write.length);
        }
        return joined.toByteArray();
    }
}
