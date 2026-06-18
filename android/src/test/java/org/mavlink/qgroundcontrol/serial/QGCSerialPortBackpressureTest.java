package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import org.junit.After;
import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.EnumSet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

public class QGCSerialPortBackpressureTest {

    private static final int CHUNK = 8;

    private QGCSerialPort port;

    @After
    public void tearDown() {
        if (port != null) {
            port.close(QGCSerialPort.CloseReason.USER);
        }
    }

    private static final class CountingBridge implements NativeBridge {
        final AtomicLong totalAcked = new AtomicLong();
        final AtomicInteger ackCalls = new AtomicInteger();
        volatile CountDownLatch acks;

        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }
        @Override public void deviceBytesWritten(final long handle, final int n) {
            totalAcked.addAndGet(n);
            ackCalls.incrementAndGet();
            final CountDownLatch l = acks;
            if (l != null) {
                l.countDown();
            }
        }
        @Override public void deviceException(final long handle, final int kind, final String message) { }
        @Override public void deviceHasDisconnected(final long handle) { }
    }

    private QGCSerialPort start(final UsbSerialPort fake, final NativeBridge bridge) {
        port = new QGCSerialPort(null, null, null, fake, 0x1L, null, null);
        port.setNativeBridgeForTest(bridge);
        port.forceConfiguredForTest(57600);
        return port;
    }

    private static ByteBuffer payload(final int length) {
        return ByteBuffer.allocate(length);
    }

    @Test
    public void invalidRequest_returnsMinusOne() {
        start(new GatedPort(new CountDownLatch(0)), new CountingBridge());
        assertEquals(-1, port.enqueueWrite(null, CHUNK));
        assertEquals(-1, port.enqueueWrite(payload(CHUNK), 0));
        assertEquals(-1, port.enqueueWrite(payload(CHUNK), -1));
    }

    @Test
    public void fullQueueIsBackpressure_acceptsAfterWriterDrains() throws Exception {
        final int total = QGCSerialPort.WRITE_QUEUE_CAPACITY + 2;
        final CountingBridge bridge = new CountingBridge();
        bridge.acks = new CountDownLatch(total);
        final CountDownLatch release = new CountDownLatch(1);
        final GatedPort fake = new GatedPort(release);

        start(fake, bridge);

        assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        assertTrue(fake.firstWriteEntered.await(5, TimeUnit.SECONDS));

        for (int i = 0; i < QGCSerialPort.WRITE_QUEUE_CAPACITY; i++) {
            assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        }

        final AtomicInteger overflowResult = new AtomicInteger(Integer.MIN_VALUE);
        final Thread overflow = new Thread(() ->
                overflowResult.set(port.enqueueWrite(payload(CHUNK), CHUNK)));
        overflow.start();

        release.countDown();
        overflow.join(5000);

        assertEquals(CHUNK, overflowResult.get());
        assertTrue(bridge.acks.await(5, TimeUnit.SECONDS));
        assertEquals((long) total * CHUNK, bridge.totalAcked.get());
        assertEquals(total, bridge.ackCalls.get());
    }

    @Test
    public void fullQueuePastTimeout_returnsMinusOne() throws Exception {
        final CountDownLatch neverReleased = new CountDownLatch(1);
        final GatedPort fake = new GatedPort(neverReleased);
        start(fake, new CountingBridge());

        assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        assertTrue(fake.firstWriteEntered.await(5, TimeUnit.SECONDS));
        for (int i = 0; i < QGCSerialPort.WRITE_QUEUE_CAPACITY; i++) {
            assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        }

        assertEquals(-1, port.enqueueWrite(payload(CHUNK), CHUNK));

        neverReleased.countDown();
    }

    @Test
    public void closeDuringBlockedEnqueue_returnsMinusOneAndDropsPayload() throws Exception {
        final CountDownLatch release = new CountDownLatch(1);
        final GatedPort fake = new GatedPort(release);
        final CountingBridge bridge = new CountingBridge();
        start(fake, bridge);

        assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        assertTrue(fake.firstWriteEntered.await(5, TimeUnit.SECONDS));
        for (int i = 0; i < QGCSerialPort.WRITE_QUEUE_CAPACITY; i++) {
            assertEquals(CHUNK, port.enqueueWrite(payload(CHUNK), CHUNK));
        }

        final AtomicInteger result = new AtomicInteger(Integer.MIN_VALUE);
        final CountDownLatch done = new CountDownLatch(1);
        final Thread blocked = new Thread(() -> {
            result.set(port.enqueueWrite(payload(CHUNK), CHUNK));
            done.countDown();
        });
        blocked.start();

        port.close(QGCSerialPort.CloseReason.USER);

        assertTrue(done.await(5, TimeUnit.SECONDS));
        assertEquals(-1, result.get());
        assertEquals(0, bridge.ackCalls.get());

        release.countDown();
    }

    private static final class GatedPort implements UsbSerialPort {
        final CountDownLatch firstWriteEntered = new CountDownLatch(1);
        private final CountDownLatch release;
        private volatile boolean open = true;

        GatedPort(final CountDownLatch release) {
            this.release = release;
        }

        @Override public void write(final byte[] src, final int length, final int timeout) throws IOException {
            firstWriteEntered.countDown();
            try {
                release.await();
            } catch (final InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new IOException(e);
            }
        }

        @Override public boolean isOpen() { return open; }
        @Override public void close() { open = false; }

        @Override public UsbSerialDriver getDriver() { return null; }
        @Override public UsbDevice getDevice() { return null; }
        @Override public int getPortNumber() { return 0; }
        @Override public UsbEndpoint getWriteEndpoint() { return null; }
        @Override public UsbEndpoint getReadEndpoint() { return null; }
        @Override public String getSerial() { return null; }
        @Override public void setReadQueue(final int count, final int size) { }
        @Override public int getReadQueueBufferCount() { return 0; }
        @Override public int getReadQueueBufferSize() { return 0; }
        @Override public void open(final UsbDeviceConnection connection) { }
        @Override public int read(final byte[] dest, final int timeout) { return 0; }
        @Override public int read(final byte[] dest, final int offset, final int timeout) { return 0; }
        @Override public void write(final byte[] src, final int timeout) { }
        @Override public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) { }
        @Override public boolean getCD() { return false; }
        @Override public boolean getCTS() { return false; }
        @Override public boolean getDSR() { return false; }
        @Override public boolean getDTR() { return false; }
        @Override public void setDTR(final boolean value) { }
        @Override public boolean getRI() { return false; }
        @Override public boolean getRTS() { return false; }
        @Override public void setRTS(final boolean value) { }
        @Override public EnumSet<ControlLine> getControlLines() { return EnumSet.noneOf(ControlLine.class); }
        @Override public EnumSet<ControlLine> getSupportedControlLines() { return EnumSet.noneOf(ControlLine.class); }
        @Override public void setFlowControl(final FlowControl flowControl) { }
        @Override public FlowControl getFlowControl() { return FlowControl.NONE; }
        @Override public EnumSet<FlowControl> getSupportedFlowControl() { return EnumSet.of(FlowControl.NONE); }
        @Override public boolean getXON() { return false; }
        @Override public void purgeHwBuffers(final boolean purgeWriteBuffers, final boolean purgeReadBuffers) { }
        @Override public void setBreak(final boolean value) { }
    }
}
