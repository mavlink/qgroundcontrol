package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class QGCUsbSerialTraceTest {
    @Test
    public void hexOf_showsEachByteAsTwoDigits() {
        final byte[] data = {0x00, 0x0A, 0x7F, (byte) 0x80, (byte) 0xFF, 0x55};
        assertEquals("00 0A 7F 80 FF ", QGCUsbSerialTrace.hexOf(data, 5));
        assertEquals("", QGCUsbSerialTrace.hexOf(data, 0));
    }

    @Test
    public void hexOf_cutsALongTransferShort() {
        final byte[] data = new byte[33];
        data[31] = 0x1F;
        data[32] = 0x20;
        assertEquals("00 ".repeat(31) + "1F ", QGCUsbSerialTrace.hexOf(data, 32));
        assertEquals("00 ".repeat(31) + "1F ...", QGCUsbSerialTrace.hexOf(data, 33));
    }
}
