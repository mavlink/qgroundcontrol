package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.UsbDevice;

import org.mavlink.qgroundcontrol.serial.QGCFtdiSerialPort.QGCFtdiSerialDriver;

import com.hoho.android.usbserial.driver.UsbSerialPort;

import org.junit.Test;

import java.util.List;

public class QGCFtdiSerialDriverTest {

    @Test
    public void buildsOnePortPerInterface() {
        final QGCFtdiSerialDriver driver = new QGCFtdiSerialDriver(new FakeUsbDevice(3));

        final List<UsbSerialPort> ports = driver.getPorts();
        assertEquals(3, ports.size());
        for (int i = 0; i < ports.size(); i++) {
            assertEquals(i, ports.get(i).getPortNumber());
            assertSame(driver, ports.get(i).getDriver());
        }
    }

    @Test
    public void zeroInterfaces_yieldsEmptyPorts() {
        final QGCFtdiSerialDriver driver = new QGCFtdiSerialDriver(new FakeUsbDevice(0));
        assertTrue(driver.getPorts().isEmpty());
    }

    @Test
    public void negativeInterfaceCount_yieldsEmptyPorts() {
        final QGCFtdiSerialDriver driver = new QGCFtdiSerialDriver(new FakeUsbDevice(-1));
        assertTrue(driver.getPorts().isEmpty());
    }

    @Test
    public void getPorts_isUnmodifiable() {
        final QGCFtdiSerialDriver driver = new QGCFtdiSerialDriver(new FakeUsbDevice(2));
        assertThrows(UnsupportedOperationException.class, () -> driver.getPorts().clear());
    }

    @Test
    public void getDevice_isPassthrough() {
        final UsbDevice device = new FakeUsbDevice(1);
        assertSame(device, new QGCFtdiSerialDriver(device).getDevice());
    }
}
