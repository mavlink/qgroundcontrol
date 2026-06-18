package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.LifecycleState;
import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager.SerialParameters;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class QGCSerialPortStateTest {

    private static final int LOW_BAUD = 57600;
    private static final int HIGH_BAUD = 921600;

    private static ByteBuffer payload(final int length) {
        return ByteBuffer.allocate(length);
    }

    private static String key(final LifecycleState from, final LifecycleState to) {
        return from + "->" + to;
    }

    @Test
    public void onlyForwardTransitionsAllowed() {
        final Set<String> allowed = Set.of(
                key(LifecycleState.REGISTERED, LifecycleState.CONFIGURED),
                key(LifecycleState.REGISTERED, LifecycleState.CLOSING),
                key(LifecycleState.CONFIGURED, LifecycleState.CLOSING),
                key(LifecycleState.CLOSING, LifecycleState.CLOSED));

        for (final LifecycleState from : LifecycleState.values()) {
            for (final LifecycleState to : LifecycleState.values()) {
                final boolean expected = allowed.contains(key(from, to));
                assertTrue(key(from, to) + " expected=" + expected,
                        expected == QGCSerialPort.isLifecycleTransitionAllowed(from, to));
            }
        }
    }

    @Test
    public void closedIsTerminal() {
        for (final LifecycleState to : LifecycleState.values()) {
            assertFalse(QGCSerialPort.isLifecycleTransitionAllowed(LifecycleState.CLOSED, to));
        }
    }

    @Test
    public void noSelfTransitionAllowed() {
        for (final LifecycleState s : LifecycleState.values()) {
            assertFalse(QGCSerialPort.isLifecycleTransitionAllowed(s, s));
        }
    }

    private static final class NoopBridge implements NativeBridge {
        volatile CountDownLatch ackLatch;
        @Override public void deviceHasDisconnected(final long handle) { }
        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }
        @Override public void deviceBytesWritten(final long handle, final int n) {
            final CountDownLatch l = ackLatch;
            if (l != null) {
                l.countDown();
            }
        }
        @Override public void deviceException(final long handle, final int kind, final String message) { }
    }

    @Test
    public void enqueueWrite_inRegisteredState_returnsMinusOne() {
        final FakeUsbSerialPort fake = new FakeUsbSerialPort(FakeUsbSerialPort.noop());
        final QGCSerialPort port = new QGCSerialPort(null, null, null, fake, 0x1L, null, null);
        port.setNativeBridgeForTest(new NoopBridge());

        assertEquals(-1, port.enqueueWrite(payload(64), 64));

        port.close(QGCSerialPort.CloseReason.USER);
    }

    @Test
    public void baudChange_genericPort_keepsMaxChunkSubWriteLength() throws Exception {
        final FakeUsbSerialPort fake = new FakeUsbSerialPort(FakeUsbSerialPort.noop());
        final NoopBridge bridge = new NoopBridge();
        final QGCSerialPort port = new QGCSerialPort(null, null, null, fake, 0x1L, null, null);
        port.setNativeBridgeForTest(bridge);
        port.forceConfiguredForTest(LOW_BAUD);

        bridge.ackLatch = new CountDownLatch(1);
        final int len = SerialWireConstants.MAX_CHUNK_BYTES + 4096;
        assertEquals(len, port.enqueueWrite(payload(len), len));
        assertTrue(bridge.ackLatch.await(5, TimeUnit.SECONDS));

        assertTrue(port.setSerialParameters(new SerialParameters(HIGH_BAUD, 8, 1, 0)));

        bridge.ackLatch = new CountDownLatch(1);
        assertEquals(len, port.enqueueWrite(payload(len), len));
        assertTrue(bridge.ackLatch.await(5, TimeUnit.SECONDS));

        port.close(QGCSerialPort.CloseReason.USER);

        for (final int sub : new ArrayList<>(fake.writeLengths())) {
            assertTrue(sub <= SerialWireConstants.MAX_CHUNK_BYTES);
        }
    }

    private static final class MutedRecordingBridge implements NativeBridge {
        final List<Integer> chunkSizes = Collections.synchronizedList(new ArrayList<>());
        int exceptionCalls;
        int disconnectCalls;

        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) {
            chunkSizes.add(length);
        }
        @Override public void deviceBytesWritten(final long handle, final int n) { }
        @Override public void deviceException(final long handle, final int kind, final String message) { exceptionCalls++; }
        @Override public void deviceHasDisconnected(final long handle) { disconnectCalls++; }
    }

    private static QGCSerialPort mutedOpenPort(final MutedRecordingBridge bridge) {
        final QGCSerialPort port = new QGCSerialPort(null, null, null, null, 0x1L, null, null);
        port.setNativeBridgeForTest(bridge);
        port.forceConfiguredForTest(57600);
        port.muteListenerForTest();
        return port;
    }

    @Test
    public void onNewData_whenMutedButOpen_emitsNothing() {
        final MutedRecordingBridge bridge = new MutedRecordingBridge();
        final QGCSerialPort port = mutedOpenPort(bridge);

        port.readLoopForTest().onNewData(new byte[128]);

        assertTrue(bridge.chunkSizes.isEmpty());
        port.close(QGCSerialPort.CloseReason.USER);
    }

    @Test
    public void runError_whenMutedButOpen_firesNoException() {
        final MutedRecordingBridge bridge = new MutedRecordingBridge();
        final QGCSerialPort port = mutedOpenPort(bridge);

        port.readLoopForTest().onRunError(new java.io.IOException("device lost"));

        assertEquals(0, bridge.exceptionCalls);
        port.close(QGCSerialPort.CloseReason.USER);
    }

    private static final class DisconnectCountingBridge implements NativeBridge {
        int disconnectCalls;

        @Override public void deviceHasDisconnected(final long handle) { disconnectCalls++; }
        @Override public void deviceException(final long handle, final int kind, final String message) { }
        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }
        @Override public void deviceBytesWritten(final long handle, final int n) { }
    }

    private static QGCSerialPort newPort(final long handle, final NativeBridge bridge) {
        final QGCSerialPort port = new QGCSerialPort(null, null, null, null, handle, null, null);
        port.setNativeBridgeForTest(bridge);
        return port;
    }

    @Test
    public void doubleClose_isIdempotent() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);
        assertTrue(port.close());
        assertTrue(port.close());
        assertEquals(0, bridge.disconnectCalls);
    }

    @Test
    public void closeFromRegistered_userReason_emitsNoDisconnect() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);

        port.close(QGCSerialPort.CloseReason.USER);

        assertEquals(0, bridge.disconnectCalls);
    }

    @Test
    public void closeFromConfigured_detached_emitsDisconnectExactlyOnce() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);
        port.forceConfiguredForTest(57600);

        port.close(QGCSerialPort.CloseReason.DETACHED);
        port.close(QGCSerialPort.CloseReason.DETACHED);

        assertEquals(1, bridge.disconnectCalls);
    }

    @Test
    public void closeFromConfigured_staleDriver_emitsDisconnectExactlyOnce() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);
        port.forceConfiguredForTest(57600);

        port.close(QGCSerialPort.CloseReason.STALE_DRIVER);
        port.close(QGCSerialPort.CloseReason.STALE_DRIVER);

        assertEquals(1, bridge.disconnectCalls);
    }

    @Test
    public void closeFromConfigured_deviceError_emitsDisconnectExactlyOnce() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);
        port.forceConfiguredForTest(57600);

        port.close(QGCSerialPort.CloseReason.DEVICE_ERROR);
        port.close(QGCSerialPort.CloseReason.DEVICE_ERROR);

        assertEquals(1, bridge.disconnectCalls);
    }

    @Test
    public void closeFromRegistered_detached_emitsDisconnectExactlyOnce() {
        final DisconnectCountingBridge bridge = new DisconnectCountingBridge();
        final QGCSerialPort port = newPort(0x1L, bridge);

        port.close(QGCSerialPort.CloseReason.DETACHED);

        assertEquals(1, bridge.disconnectCalls);
    }

    private static final class ExceptionRecordingBridge implements NativeBridge {
        long exceptionHandle = -1;
        int exceptionKind = -1;
        String exceptionMessage;
        boolean exceptionCalled;

        @Override public void deviceHasDisconnected(final long handle) { }
        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }
        @Override public void deviceBytesWritten(final long handle, final int n) { }

        @Override public void deviceException(final long handle, final int kind, final String message) {
            exceptionCalled = true;
            exceptionHandle = handle;
            exceptionKind = kind;
            exceptionMessage = message;
        }
    }

    private static QGCSerialPort newPortWithHandle(final long handle) {
        return new QGCSerialPort(null, null, null, null, handle, null, null);
    }

    @Test
    public void fireException_routesThroughBridgeWithLiveHandle() {
        final QGCSerialPort port = newPortWithHandle(0x1234L);
        final ExceptionRecordingBridge bridge = new ExceptionRecordingBridge();
        port.setNativeBridgeForTest(bridge);

        port.fireException(SerialWireConstants.EXC_RESOURCE, "boom");

        assertEquals(0x1234L, bridge.exceptionHandle);
        assertEquals(SerialWireConstants.EXC_RESOURCE, bridge.exceptionKind);
        assertEquals("boom", bridge.exceptionMessage);
    }

    @Test
    public void fireException_suppressedWhenHandleZero() {
        final QGCSerialPort port = newPortWithHandle(0L);
        final ExceptionRecordingBridge bridge = new ExceptionRecordingBridge();
        port.setNativeBridgeForTest(bridge);

        port.fireException(SerialWireConstants.EXC_UNKNOWN, "ignored");

        assertFalse(bridge.exceptionCalled);
    }
}
