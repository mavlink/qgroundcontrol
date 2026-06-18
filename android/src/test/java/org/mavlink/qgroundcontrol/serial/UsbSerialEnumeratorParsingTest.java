package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class UsbSerialEnumeratorParsingTest {

    @Test
    public void nullOrEmpty_yieldsEmptyBasePortZero() {
        UsbSerialEnumerator.DevicePortSpec s = UsbSerialEnumerator.parseDevicePortSpec(null);
        assertEquals("", s.baseDeviceName());
        assertEquals(0, s.portIndex());

        s = UsbSerialEnumerator.parseDevicePortSpec("");
        assertEquals("", s.baseDeviceName());
        assertEquals(0, s.portIndex());
    }

    @Test
    public void noSuffix_returnsWholeNamePortZero() {
        final UsbSerialEnumerator.DevicePortSpec s =
                UsbSerialEnumerator.parseDevicePortSpec("/dev/bus/usb/001/002");
        assertEquals("/dev/bus/usb/001/002", s.baseDeviceName());
        assertEquals(0, s.portIndex());
    }

    @Test
    public void validSuffix_splitsBaseAndIndex() {
        final UsbSerialEnumerator.DevicePortSpec s =
                UsbSerialEnumerator.parseDevicePortSpec("/dev/bus/usb/001/002" + UsbSerialEnumerator.PORT_SUFFIX + "2");
        assertEquals("/dev/bus/usb/001/002", s.baseDeviceName());
        assertEquals(2, s.portIndex());
    }

    @Test
    public void malformedSuffix_fallsBackToWholeNamePortZero() {
        final UsbSerialEnumerator.DevicePortSpec s =
                UsbSerialEnumerator.parseDevicePortSpec("/dev/x" + UsbSerialEnumerator.PORT_SUFFIX + "ZZ");
        assertEquals("/dev/x" + UsbSerialEnumerator.PORT_SUFFIX + "ZZ", s.baseDeviceName());
        assertEquals(0, s.portIndex());
    }

    @Test
    public void suffixAtStart_treatedAsNoSplit() {
        final UsbSerialEnumerator.DevicePortSpec s =
                UsbSerialEnumerator.parseDevicePortSpec(UsbSerialEnumerator.PORT_SUFFIX + "2");
        assertEquals(UsbSerialEnumerator.PORT_SUFFIX + "2", s.baseDeviceName());
        assertEquals(0, s.portIndex());
    }

    @Test
    public void negativeSuffix_clampedToZero() {
        final UsbSerialEnumerator.DevicePortSpec s =
                UsbSerialEnumerator.parseDevicePortSpec("name" + UsbSerialEnumerator.PORT_SUFFIX + "-1");
        assertEquals("name", s.baseDeviceName());
        assertEquals(0, s.portIndex());
    }

    @Test
    public void devicePortSpec_clampsNegativeIndex() {
        assertEquals(0, UsbSerialEnumerator.DevicePortSpec.of("x", -5).portIndex());
        assertEquals(3, UsbSerialEnumerator.DevicePortSpec.of("x", 3).portIndex());
    }

    @Test
    public void getPortFromNullDriver_returnsNull() {
        assertNull(UsbSerialEnumerator.getPortFromDriver(null, 0));
    }
}
