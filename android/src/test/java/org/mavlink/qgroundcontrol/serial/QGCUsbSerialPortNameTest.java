package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;

import org.junit.Test;

public class QGCUsbSerialPortNameTest {
    private static final String DEVICE_NAME = "/dev/bus/usb/001/002";

    @Test
    public void parse_withoutSuffix_defaultsToPortZero() {
        assertEquals(new QGCUsbSerialPortName(DEVICE_NAME, 0), QGCUsbSerialPortName.parse(DEVICE_NAME));
    }

    @Test
    public void parse_withSuffix_extractsPortIndex() {
        assertEquals(new QGCUsbSerialPortName(DEVICE_NAME, 3), QGCUsbSerialPortName.parse(DEVICE_NAME + "#p3"));
    }

    @Test
    public void parse_invalidSuffix_fallsBackToOriginalNameAndPortZero() {
        final String malformed = DEVICE_NAME + "#pabc";
        assertEquals(new QGCUsbSerialPortName(malformed, 0), QGCUsbSerialPortName.parse(malformed));
    }

    @Test
    public void isStable_tellsAStableNameFromTheOneAndroidAssigns() {
        final QGCUsbSerialPortName stable = QGCUsbSerialPortName.parse("usb:0403:6010:A1#p1");
        assertEquals(new QGCUsbSerialPortName("usb:0403:6010:A1", 1), stable);
        assertTrue(stable.isStable());

        assertFalse(QGCUsbSerialPortName.parse(DEVICE_NAME + "#p1").isStable());
    }

    @Test
    public void format_addsThePortSuffixOnlyForAMultiPortDevice() {
        assertEquals(DEVICE_NAME, new QGCUsbSerialPortName(DEVICE_NAME, 0).format(1));
        assertEquals(DEVICE_NAME + "#p0", new QGCUsbSerialPortName(DEVICE_NAME, 0).format(2));
        assertEquals(DEVICE_NAME + "#p1", new QGCUsbSerialPortName(DEVICE_NAME, 1).format(2));
    }

    @Test
    public void stableDeviceName_padsTheIdsToFourLowerCaseHexDigits() {
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        device.vendorId = 0x26AC;
        device.productId = 0x11;
        assertEquals("usb:26ac:0011:A1", QGCUsbSerialPortName.stableDeviceName(device, "A1"));
    }

    @Test
    public void stableDeviceName_rewritesTheSuffixCharacterInASerialNumber() {
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        assertEquals("usb:0000:0000:AB%23p1", QGCUsbSerialPortName.stableDeviceName(device, "AB#p1"));
        // What marks the rewriting is rewritten too, so that no two serial numbers share a name.
        assertEquals("usb:0000:0000:AB%2523p1", QGCUsbSerialPortName.stableDeviceName(device, "AB%23p1"));
    }

    @Test
    public void stableDeviceName_rewritesWhitespaceInASerialNumber() {
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        // A link's port name loses the whitespace at its ends when it is stored, and must stay the same name.
        assertEquals("usb:0000:0000:AB12%20", QGCUsbSerialPortName.stableDeviceName(device, "AB12 "));
        assertEquals("usb:0000:0000:A%09B%u2003", QGCUsbSerialPortName.stableDeviceName(device, "A\tB\u2003"));
        // Apart from the serial number that spells the same digits.
        assertEquals("usb:0000:0000:%2003", QGCUsbSerialPortName.stableDeviceName(device, " 03"));
    }

    @Test
    public void stableDeviceName_withoutSerialNumber_hasAnEmptySerialPart() {
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        assertEquals("usb:0000:0000:", QGCUsbSerialPortName.stableDeviceName(device, null));
    }
}
