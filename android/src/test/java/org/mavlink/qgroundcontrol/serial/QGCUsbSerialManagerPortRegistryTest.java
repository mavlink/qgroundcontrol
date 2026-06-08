package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import org.mavlink.qgroundcontrol.serial.QGCSerialPort.NativeBridge;
import org.mavlink.qgroundcontrol.serial.QGCSerialPort.PortLifecycleSink;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager.PortAddress;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager.PortRegistry;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class QGCUsbSerialManagerPortRegistryTest {

    private static final class RecordingBridge implements NativeBridge {
        int exceptionCalls;
        int lastExceptionKind = Integer.MIN_VALUE;

        @Override public void deviceHasDisconnected(final long handle) { }
        @Override public void deviceNewData(final long handle, final ByteBuffer data, final int length) { }
        @Override public void deviceBytesWritten(final long handle, final int n) { }
        @Override public void deviceException(final long handle, final int kind, final String message) {
            exceptionCalls++;
            lastExceptionKind = kind;
        }
    }

    private static QGCSerialPort port(final String deviceName, final int physicalId, final int portIndex,
            final long handle, final RecordingBridge bridge) {
        final PortAddress address = new PortAddress(deviceName, physicalId, portIndex);
        final QGCSerialPort p = new QGCSerialPort(address, null, null, null, handle, null, null);
        if (bridge != null) {
            p.setNativeBridgeForTest(bridge);
        }
        return p;
    }

    @Test
    public void register_putIfAbsentRejectsDuplicateAddress() {
        final PortRegistry registry = new PortRegistry();
        final QGCSerialPort first = port("dev/a", 1, 0, 0x1L, null);
        final QGCSerialPort dup = port("dev/a", 1, 0, 0x2L, null);

        assertTrue(registry.register(first));
        assertFalse(registry.register(dup));
        assertSame(first, registry.get(new PortAddress("dev/a", 1, 0)));
    }

    @Test
    public void unregister_removesOnlyOnValueMatch() {
        final PortRegistry registry = new PortRegistry();
        final QGCSerialPort registered = port("dev/a", 1, 0, 0x1L, null);
        final QGCSerialPort stranger = port("dev/a", 1, 0, 0x2L, null);
        registry.register(registered);

        registry.unregister(stranger);
        assertSame(registered, registry.get(new PortAddress("dev/a", 1, 0)));

        registry.unregister(registered);
        assertNull(registry.get(new PortAddress("dev/a", 1, 0)));
    }

    @Test
    public void portsForDeviceName_filtersByDeviceName() {
        final PortRegistry registry = new PortRegistry();
        final QGCSerialPort a0 = port("dev/a", 1, 0, 0x1L, null);
        final QGCSerialPort a1 = port("dev/a", 1, 1, 0x2L, null);
        final QGCSerialPort b0 = port("dev/b", 2, 0, 0x3L, null);
        registry.register(a0);
        registry.register(a1);
        registry.register(b0);

        final List<QGCSerialPort> matches = registry.portsForDeviceName("dev/a");

        assertEquals(2, matches.size());
        assertTrue(matches.contains(a0));
        assertTrue(matches.contains(a1));
        assertFalse(matches.contains(b0));
    }

    @Test
    public void portsForDeviceName_returnsEmptyWhenNoMatch() {
        final PortRegistry registry = new PortRegistry();
        registry.register(port("dev/a", 1, 0, 0x1L, null));

        assertTrue(registry.portsForDeviceName("dev/missing").isEmpty());
    }

    @Test
    public void allPorts_returnsEveryRegisteredPort() {
        final PortRegistry registry = new PortRegistry();
        final QGCSerialPort a = port("dev/a", 1, 0, 0x1L, null);
        final QGCSerialPort b = port("dev/b", 2, 0, 0x2L, null);
        registry.register(a);
        registry.register(b);

        final List<QGCSerialPort> all = new ArrayList<>(registry.allPorts());

        assertEquals(2, all.size());
        assertTrue(all.contains(a));
        assertTrue(all.contains(b));
    }

    @Test
    public void close_clearsRegistryBeforeOnPortClosedFires() {
        final PortRegistry registry = new PortRegistry();
        final PortAddress address = new PortAddress("dev/a", 1, 0);
        final boolean[] emptyWhenClosed = { false };
        final boolean[] onPortClosedFired = { false };

        final QGCSerialPort[] holder = new QGCSerialPort[1];
        final PortLifecycleSink sink = new PortLifecycleSink() {
            @Override
            public void onPortConfigured(final QGCSerialPort port) {}

            @Override
            public void onPortClosed(final QGCSerialPort port) {
                onPortClosedFired[0] = true;
                // C++ unregisterPort runs LAST (after this callback); the Java registry must already be cleared here.
                emptyWhenClosed[0] = registry.get(address) == null;
            }

            @Override
            public void onPortDeviceError(final QGCSerialPort port) {}
        };
        final QGCSerialPort p = new QGCSerialPort(address, null, null, null, 0x42L, sink,
                () -> registry.unregister(holder[0]));
        holder[0] = p;

        assertTrue(registry.register(p));
        assertSame(p, registry.get(address));

        p.forceConfiguredForTest(57600);
        assertTrue(p.close());

        assertTrue(onPortClosedFired[0]);
        assertTrue(emptyWhenClosed[0]);
        assertNull(registry.get(address));
    }

    @Test
    public void firePermissionDenied_firesExcPermissionOnEveryPortForDeviceName() {
        final PortRegistry registry = new PortRegistry();
        final RecordingBridge a0Bridge = new RecordingBridge();
        final RecordingBridge a1Bridge = new RecordingBridge();
        final RecordingBridge b0Bridge = new RecordingBridge();
        registry.register(port("dev/a", 1, 0, 0x10L, a0Bridge));
        registry.register(port("dev/a", 1, 1, 0x11L, a1Bridge));
        registry.register(port("dev/b", 2, 0, 0x20L, b0Bridge));

        QGCUsbSerialManager.firePermissionDeniedForDeviceName(registry, "dev/a");

        assertEquals(1, a0Bridge.exceptionCalls);
        assertEquals(SerialWireConstants.EXC_PERMISSION, a0Bridge.lastExceptionKind);
        assertEquals(1, a1Bridge.exceptionCalls);
        assertEquals(SerialWireConstants.EXC_PERMISSION, a1Bridge.lastExceptionKind);
        assertEquals(0, b0Bridge.exceptionCalls);
    }
}
