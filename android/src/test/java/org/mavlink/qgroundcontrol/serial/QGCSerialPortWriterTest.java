package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.hoho.android.usbserial.driver.SerialTimeoutException;

import org.junit.After;
import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

public class QGCSerialPortWriterTest {

    private QGCSerialPort port;

    @After
    public void tearDown() {
        if (port != null) {
            port.close(QGCSerialPort.CloseReason.USER);
        }
    }

    private static final class RecordingBridge implements NativeBridge {
        final AtomicLong totalAcked = new AtomicLong();
        final AtomicInteger ackCalls = new AtomicInteger();
        final AtomicInteger exceptionCalls = new AtomicInteger();
        final AtomicInteger lastExceptionKind = new AtomicInteger(Integer.MIN_VALUE);
        final List<Integer> ackSizes = Collections.synchronizedList(new ArrayList<>());
        volatile CountDownLatch latch;

        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }

        @Override public void deviceBytesWritten(final long handle, final int n) {
            totalAcked.addAndGet(n);
            ackCalls.incrementAndGet();
            ackSizes.add(n);
            trip();
        }

        @Override public void deviceException(final long handle, final int kind, final String message) {
            lastExceptionKind.set(kind);
            exceptionCalls.incrementAndGet();
            trip();
        }

        @Override public void deviceHasDisconnected(final long handle) {
            trip();
        }

        private void trip() {
            final CountDownLatch l = latch;
            if (l != null) {
                l.countDown();
            }
        }
    }

    private QGCSerialPort startConfiguredPort(final FakeUsbSerialPort fake, final NativeBridge bridge) {
        port = new QGCSerialPort(null, null, null, fake, 0x1L, null, null);
        port.setNativeBridgeForTest(bridge);
        port.forceConfiguredForTest(57600);
        return port;
    }

    private static ByteBuffer payload(final int length) {
        return ByteBuffer.allocate(length);
    }

    @Test
    public void fullWrite_acksExactlyRequestedBytes() throws Exception {
        final RecordingBridge bridge = new RecordingBridge();
        bridge.latch = new CountDownLatch(1);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort((data, length, call) -> { });

        startConfiguredPort(fake, bridge);
        assertEquals(256, port.enqueueWrite(payload(256), 256));

        assertTrue(bridge.latch.await(5, TimeUnit.SECONDS));
        assertEquals(256L, bridge.totalAcked.get());
        assertEquals(1, bridge.ackCalls.get());
    }

    @Test
    public void partialStallThenAccept_acksOnlyTransferredBytesPerSubwrite() throws Exception {
        final RecordingBridge bridge = new RecordingBridge();
        bridge.latch = new CountDownLatch(2);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort((data, length, call) -> {
            if (call == 0) {
                throw new SerialTimeoutException("stalled", 100);
            }
        });

        startConfiguredPort(fake, bridge);
        assertEquals(256, port.enqueueWrite(payload(256), 256));

        assertTrue(bridge.latch.await(5, TimeUnit.SECONDS));
        assertEquals(256L, bridge.totalAcked.get());
        assertEquals(2, bridge.ackCalls.get());
        assertEquals(List.of(100, 156), bridge.ackSizes);
    }

    @Test
    public void writeIoException_firesResourceExceptionAndAcksNothing() throws Exception {
        final RecordingBridge bridge = new RecordingBridge();
        bridge.latch = new CountDownLatch(1);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort((data, length, call) -> {
            throw new IOException("device lost");
        });

        startConfiguredPort(fake, bridge);
        assertEquals(64, port.enqueueWrite(payload(64), 64));

        assertTrue(bridge.latch.await(5, TimeUnit.SECONDS));
        assertEquals(1, bridge.exceptionCalls.get());
        assertEquals(SerialWireConstants.EXC_RESOURCE, bridge.lastExceptionKind.get());
        assertEquals(0, bridge.ackCalls.get());
    }

    @Test
    public void enqueueAfterClose_returnsMinusOne() {
        final RecordingBridge bridge = new RecordingBridge();
        final FakeUsbSerialPort fake = new FakeUsbSerialPort((data, length, call) -> { });

        startConfiguredPort(fake, bridge);
        port.close(QGCSerialPort.CloseReason.USER);

        assertEquals(-1, port.enqueueWrite(payload(32), 32));
    }
}
