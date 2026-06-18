package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.hoho.android.usbserial.driver.SerialTimeoutException;

import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;
import org.mavlink.qgroundcontrol.serial.QGCSerialPort.PortLifecycleSink;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class QGCSerialPortReadPathTest {

    private static final class RecordingBridge implements NativeBridge {
        final List<Integer> chunkSizes = new ArrayList<>();
        int exceptionCalls;
        int lastExceptionKind = Integer.MIN_VALUE;
        int disconnectCalls;

        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) {
            chunkSizes.add(length);
        }
        @Override public void deviceBytesWritten(final long handle, final int n) { }
        @Override public void deviceException(final long handle, final int kind, final String message) {
            exceptionCalls++;
            lastExceptionKind = kind;
        }
        @Override public void deviceHasDisconnected(final long handle) { disconnectCalls++; }

        int totalBytes() {
            int sum = 0;
            for (final int n : chunkSizes) {
                sum += n;
            }
            return sum;
        }
    }

    private static final class RecordingSink implements PortLifecycleSink {
        int deviceErrorCalls;
        @Override public void onPortConfigured(final QGCSerialPort port) { }
        @Override public void onPortClosed(final QGCSerialPort port) { }
        @Override public void onPortDeviceError(final QGCSerialPort port) { deviceErrorCalls++; }
    }

    private static QGCSerialPort newPort(final NativeBridge bridge, final PortLifecycleSink sink) {
        final QGCSerialPort port = new QGCSerialPort(null, null, null, null, 0x1L, sink, null);
        port.setNativeBridgeForTest(bridge);
        return port;
    }

    @Test
    public void smallPayload_emitsSingleChunk() {
        final RecordingBridge bridge = new RecordingBridge();
        newPort(bridge, null).readLoopForTest().onNewData(new byte[256]);

        assertEquals(List.of(256), bridge.chunkSizes);
        assertEquals(256, bridge.totalBytes());
    }

    @Test
    public void payloadAtChunkLimit_emitsSingleChunk() {
        final RecordingBridge bridge = new RecordingBridge();
        newPort(bridge, null).readLoopForTest().onNewData(new byte[SerialWireConstants.MAX_CHUNK_BYTES]);

        assertEquals(List.of(SerialWireConstants.MAX_CHUNK_BYTES), bridge.chunkSizes);
    }

    @Test
    public void largePayload_chunkedAtMaxChunkBytesPreservingTotal() {
        final RecordingBridge bridge = new RecordingBridge();
        final int total = (2 * SerialWireConstants.MAX_CHUNK_BYTES) + 7232;
        newPort(bridge, null).readLoopForTest().onNewData(new byte[total]);

        assertEquals(List.of(SerialWireConstants.MAX_CHUNK_BYTES, SerialWireConstants.MAX_CHUNK_BYTES, 7232),
                bridge.chunkSizes);
        assertEquals(total, bridge.totalBytes());
    }

    @Test
    public void nullOrEmptyPayload_emitsNothing() {
        final RecordingBridge bridge = new RecordingBridge();
        final QGCSerialPort port = newPort(bridge, null);

        port.readLoopForTest().onNewData(null);
        port.readLoopForTest().onNewData(new byte[0]);

        assertTrue(bridge.chunkSizes.isEmpty());
    }

    @Test
    public void afterClose_onNewDataEmitsNothing() {
        final RecordingBridge bridge = new RecordingBridge();
        final QGCSerialPort port = newPort(bridge, null);
        port.close(QGCSerialPort.CloseReason.USER);

        port.readLoopForTest().onNewData(new byte[128]);

        assertTrue(bridge.chunkSizes.isEmpty());
    }

    @Test
    public void runError_ioException_firesResourceAndReplacesDriver() {
        final RecordingBridge bridge = new RecordingBridge();
        final RecordingSink sink = new RecordingSink();
        newPort(bridge, sink).readLoopForTest().onRunError(new IOException("device lost"));

        assertEquals(1, bridge.exceptionCalls);
        assertEquals(SerialWireConstants.EXC_RESOURCE, bridge.lastExceptionKind);
        assertEquals(1, sink.deviceErrorCalls);
    }

    @Test
    public void runError_serialTimeout_firesUnknownAndKeepsDriver() {
        final RecordingBridge bridge = new RecordingBridge();
        final RecordingSink sink = new RecordingSink();
        newPort(bridge, sink).readLoopForTest().onRunError(new SerialTimeoutException("stalled", 0));

        assertEquals(1, bridge.exceptionCalls);
        assertEquals(SerialWireConstants.EXC_UNKNOWN, bridge.lastExceptionKind);
        assertEquals(0, sink.deviceErrorCalls);
    }

    @Test
    public void afterClose_runErrorIsSilent() {
        final RecordingBridge bridge = new RecordingBridge();
        final RecordingSink sink = new RecordingSink();
        final QGCSerialPort port = newPort(bridge, sink);
        port.close(QGCSerialPort.CloseReason.USER);

        port.readLoopForTest().onRunError(new IOException("device lost"));

        assertEquals(0, bridge.exceptionCalls);
        assertEquals(0, sink.deviceErrorCalls);
    }
}
