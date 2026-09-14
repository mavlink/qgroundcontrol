package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import java.io.IOException;
import java.lang.reflect.Proxy;
import java.util.EnumSet;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class QGCUsbSerialManagerTest {

    @Before
    public void setUp() {
        QGCUsbSerialManager.resetResourceMappingsForTesting();
    }

    @After
    public void tearDown() {
        QGCUsbSerialManager.resetResourceMappingsForTesting();
    }

    @Test
    public void parseDevicePortSpec_withoutSuffix_defaultsToPortZero() {
        final String deviceName = "/dev/bus/usb/001/002";
        assertEquals(deviceName, QGCUsbSerialManager.getBaseDeviceNameForTesting(deviceName));
        assertEquals(0, QGCUsbSerialManager.getPortIndexForTesting(deviceName));
    }

    @Test
    public void parseDevicePortSpec_withSuffix_extractsPortIndex() {
        assertEquals("/dev/bus/usb/001/002", QGCUsbSerialManager.getBaseDeviceNameForTesting("/dev/bus/usb/001/002#p3"));
        assertEquals(3, QGCUsbSerialManager.getPortIndexForTesting("/dev/bus/usb/001/002#p3"));
    }

    @Test
    public void parseDevicePortSpec_invalidSuffix_fallsBackToOriginalNameAndPortZero() {
        final String malformed = "/dev/bus/usb/001/002#pabc";
        assertEquals(malformed, QGCUsbSerialManager.getBaseDeviceNameForTesting(malformed));
        assertEquals(0, QGCUsbSerialManager.getPortIndexForTesting(malformed));
    }

    @Test
    public void resourceIdMapping_reusesSameAddressAndSeparatesPorts() {
        final int first = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 0);
        final int second = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 0);
        final int otherPort = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 1);

        assertEquals(first, second);
        assertNotEquals(first, otherPort);
    }

    @Test
    public void resourceIdMapping_removedAddressGetsNewId() {
        final int first = QGCUsbSerialManager.getOrCreateResourceIdForTesting(7, 2);
        QGCUsbSerialManager.removeResourceMappingForTesting(first);
        final int next = QGCUsbSerialManager.getOrCreateResourceIdForTesting(7, 2);

        assertNotEquals(first, next);
    }
    @Test
    public void controlLineSupport_distinguishesUnsupportedFromFailure() throws IOException {
        UsbSerialPort port = portWithControlLines(EnumSet.of(UsbSerialPort.ControlLine.DTR));
        assertEquals(1, QGCUsbSerialManager.controlLineSupport(port, UsbSerialPort.ControlLine.DTR));
        assertEquals(0, QGCUsbSerialManager.controlLineSupport(port, UsbSerialPort.ControlLine.RTS));
        port = portWithControlLines(new UnsupportedOperationException());
        assertEquals(0, QGCUsbSerialManager.controlLineSupport(port, UsbSerialPort.ControlLine.DTR));
        port = portWithControlLines(new IOException("Disconnected"));
        assertEquals(-1, QGCUsbSerialManager.controlLineSupport(port, UsbSerialPort.ControlLine.DTR));
        assertEquals(-1, QGCUsbSerialManager.controlLineSupport(null, UsbSerialPort.ControlLine.DTR));
    }

    private static UsbSerialPort portWithControlLines(final Object result) {
        return (UsbSerialPort) Proxy.newProxyInstance(UsbSerialPort.class.getClassLoader(),
                new Class<?>[] { UsbSerialPort.class }, (proxy, method, args) -> {
                    if (!method.getName().equals("getSupportedControlLines")) {
                        throw new AssertionError("Unexpected call: " + method.getName());
                    }
                    if (result instanceof Exception) {
                        throw (Exception) result;
                    }
                    return result;
                });
    }

}
