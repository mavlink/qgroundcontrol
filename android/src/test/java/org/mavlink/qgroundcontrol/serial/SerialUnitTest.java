package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import org.junit.Before;
import org.junit.Test;

public class SerialUnitTest {

    private UsbDeviceRegistry registry;

    @Before
    public void setUp() {
        registry = new UsbDeviceRegistry();
    }

    @Test
    public void parseDevicePortSpec_withoutSuffix_defaultsToPortZero() {
        final String deviceName = "/dev/bus/usb/001/002";
        final UsbSerialEnumerator.DevicePortSpec spec =
                UsbSerialEnumerator.parseDevicePortSpec(deviceName);
        assertEquals(deviceName, spec.baseDeviceName);
        assertEquals(0, spec.portIndex);
    }

    @Test
    public void parseDevicePortSpec_withSuffix_extractsPortIndex() {
        final UsbSerialEnumerator.DevicePortSpec spec =
                UsbSerialEnumerator.parseDevicePortSpec("/dev/bus/usb/001/002#p3");
        assertEquals("/dev/bus/usb/001/002", spec.baseDeviceName);
        assertEquals(3, spec.portIndex);
    }

    @Test
    public void parseDevicePortSpec_invalidSuffix_fallsBackToOriginalNameAndPortZero() {
        final String malformed = "/dev/bus/usb/001/002#pabc";
        final UsbSerialEnumerator.DevicePortSpec spec =
                UsbSerialEnumerator.parseDevicePortSpec(malformed);
        assertEquals(malformed, spec.baseDeviceName);
        assertEquals(0, spec.portIndex);
    }

    @Test
    public void resourceIdMapping_reusesSameAddressAndSeparatesPorts() {
        final int first = registry.getOrCreateHandle(42, 0);
        final int second = registry.getOrCreateHandle(42, 0);
        final int otherPort = registry.getOrCreateHandle(42, 1);

        assertEquals(first, second);
        assertNotEquals(first, otherPort);
    }

    @Test
    public void resourceIdMapping_removedAddressGetsNewId() {
        final int first = registry.getOrCreateHandle(7, 2);
        registry.remove(first);
        final int next = registry.getOrCreateHandle(7, 2);

        assertNotEquals(first, next);
    }
}
