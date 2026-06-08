package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;

import com.hoho.android.usbserial.driver.Cp21xxSerialDriver;

import org.junit.After;
import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

public class QGCSerialPortLoopbackTest {

    private static final int LOW_BAUD = 57600;
    private static final int HIGH_BAUD = 500000;

    private QGCSerialPort port;

    @After
    public void tearDown() {
        if (port != null) {
            port.close(QGCSerialPort.CloseReason.USER);
        }
    }

    private static final class RecordingBridge implements NativeBridge {
        final AtomicLong totalAcked = new AtomicLong();
        final List<byte[]> rxChunks = Collections.synchronizedList(new ArrayList<>());
        final AtomicInteger exceptionCalls = new AtomicInteger();
        final AtomicInteger disconnectCalls = new AtomicInteger();
        volatile CountDownLatch ackLatch;

        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) {
            final byte[] copy = new byte[length];
            final int pos = data.position();
            data.get(copy, 0, length);
            data.position(pos);
            rxChunks.add(copy);
        }

        @Override public void deviceBytesWritten(final long handle, final int n) {
            totalAcked.addAndGet(n);
            final CountDownLatch l = ackLatch;
            if (l != null) {
                for (int i = 0; i < n; i++) {
                    l.countDown();
                }
            }
        }

        @Override public void deviceException(final long handle, final int kind, final String message) {
            exceptionCalls.incrementAndGet();
        }

        @Override public void deviceHasDisconnected(final long handle) {
            disconnectCalls.incrementAndGet();
        }

        byte[] rxJoined() {
            synchronized (rxChunks) {
                int total = 0;
                for (final byte[] c : rxChunks) {
                    total += c.length;
                }
                final byte[] out = new byte[total];
                int off = 0;
                for (final byte[] c : rxChunks) {
                    System.arraycopy(c, 0, out, off, c.length);
                    off += c.length;
                }
                return out;
            }
        }
    }

    private static ByteBuffer ramp(final int length, final int start) {
        final ByteBuffer buf = ByteBuffer.allocate(length);
        for (int i = 0; i < length; i++) {
            buf.put((byte) ((start + i) & 0xff));
        }
        buf.flip();
        return buf;
    }

    private QGCSerialPort start(final FakeUsbSerialPort fake, final RecordingBridge bridge, final int baud) {
        final Cp21xxSerialDriver driver = new Cp21xxSerialDriver(new FakeUsbDevice(1));
        port = new QGCSerialPort(null, null, driver, fake, 0x1L, null, null);
        port.setNativeBridgeForTest(bridge);
        port.forceConfiguredForTest(baud);
        return port;
    }

    @Test
    public void roundTrip_acksEnqueuedAndSurfacedBytesPreservingOrder() throws Exception {
        final int total = SerialWireConstants.MAX_CHUNK_BYTES * 2 + 777;
        final RecordingBridge bridge = new RecordingBridge();
        bridge.ackLatch = new CountDownLatch(total);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        start(fake, bridge, LOW_BAUD);
        assertEquals(total, port.enqueueWrite(ramp(total, 0), total));

        assertTrue(bridge.ackLatch.await(5, TimeUnit.SECONDS));
        assertEquals((long) total, bridge.totalAcked.get());

        final byte[] onWire = fake.writtenBytes();
        assertEquals(total, onWire.length);

        port.readLoopForTest().onNewData(onWire);

        final byte[] rx = bridge.rxJoined();
        assertEquals(total, rx.length);
        assertArrayEquals(onWire, rx);
        final byte[] expected = new byte[total];
        for (int i = 0; i < total; i++) {
            expected[i] = (byte) (i & 0xff);
        }
        assertArrayEquals(expected, rx);
    }

    @Test
    public void baudChangeMidStream_subWriteChunkingFollowsBaud_roundTripExact() throws Exception {
        final int first = SerialWireConstants.MAX_CHUNK_BYTES + 100;
        final RecordingBridge bridge = new RecordingBridge();
        bridge.ackLatch = new CountDownLatch(first);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort(FakeUsbSerialPort.noop());

        start(fake, bridge, LOW_BAUD);
        assertEquals(first, port.enqueueWrite(ramp(first, 0), first));
        assertTrue(bridge.ackLatch.await(5, TimeUnit.SECONDS));

        final int lowChunks = fake.writeLengths().size();
        assertTrue(fake.writeLengths().contains(SerialWireConstants.MAX_CHUNK_BYTES));

        assertTrue(port.setSerialParameters(new QGCUsbSerialManager.SerialParameters(HIGH_BAUD, 8, 1, 0)));

        final int second = DriverStrategy.CP21XX_HIGH_BAUD_WRITE_CHUNK_BYTES * 3;
        bridge.ackLatch = new CountDownLatch(second);
        assertEquals(second, port.enqueueWrite(ramp(second, 7), second));
        assertTrue(bridge.ackLatch.await(5, TimeUnit.SECONDS));

        final List<Integer> highBaudWrites = fake.writeLengths().subList(lowChunks, fake.writeLengths().size());
        for (final int len : highBaudWrites) {
            assertTrue(len <= DriverStrategy.CP21XX_HIGH_BAUD_WRITE_CHUNK_BYTES);
        }
        assertEquals((long) (first + second), bridge.totalAcked.get());
        assertEquals(first + second, fake.writtenBytes().length);
    }

    @Test
    public void disconnectMidStream_emitsSingleDisconnect_andAccountingReconciles() throws Exception {
        final int total = SerialWireConstants.MAX_CHUNK_BYTES * 4;
        final RecordingBridge bridge = new RecordingBridge();
        final CountDownLatch firstWrite = new CountDownLatch(1);
        final CountDownLatch release = new CountDownLatch(1);
        final FakeUsbSerialPort fake = new FakeUsbSerialPort((data, length, call) -> {
            if (call == 0) {
                firstWrite.countDown();
                try {
                    release.await(2, TimeUnit.SECONDS);
                } catch (final InterruptedException ignored) {
                    Thread.currentThread().interrupt();
                }
            }
        });

        start(fake, bridge, HIGH_BAUD);
        assertEquals(total, port.enqueueWrite(ramp(total, 0), total));
        assertTrue(firstWrite.await(5, TimeUnit.SECONDS));

        port.close(QGCSerialPort.CloseReason.DETACHED);
        port.close(QGCSerialPort.CloseReason.DETACHED);
        release.countDown();

        assertTrue(port.awaitWriteLoopDrainedForTest(5000));
        assertEquals(1, bridge.disconnectCalls.get());
        assertEquals(0, bridge.exceptionCalls.get());
        assertTrue(bridge.totalAcked.get() <= total);
    }
}
