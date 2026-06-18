package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.UsbDevice;

import com.hoho.android.usbserial.driver.UsbSerialPort;

import org.junit.Test;

import java.io.IOException;
import java.util.EnumSet;

public class QGCFtdiSerialPortTest {

    private static QGCFtdiSerialPort closedPort() {
        final UsbDevice device = new FakeUsbDevice(1);
        final QGCFtdiSerialPort.QGCFtdiSerialDriver driver = new QGCFtdiSerialPort.QGCFtdiSerialDriver(device);
        return new QGCFtdiSerialPort(driver, device, 0);
    }

    @Test
    public void notOpen_whenConstructed() {
        assertFalse(closedPort().isOpen());
    }

    @Test
    public void getCD_whenClosed_throwsPortNotOpen() {
        final IOException e = assertThrows(IOException.class, () -> closedPort().getCD());
        assertEquals("Port not open", e.getMessage());
    }

    @Test
    public void getCTS_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getCTS()).getMessage());
    }

    @Test
    public void getDSR_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getDSR()).getMessage());
    }

    @Test
    public void getDTR_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getDTR()).getMessage());
    }

    @Test
    public void getRI_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getRI()).getMessage());
    }

    @Test
    public void getRTS_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getRTS()).getMessage());
    }

    @Test
    public void getControlLines_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getControlLines()).getMessage());
    }

    @Test
    public void getXON_whenClosed_throwsPortNotOpen() {
        assertEquals("Port not open", assertThrows(IOException.class, () -> closedPort().getXON()).getMessage());
    }

    @Test
    public void getFlowControl_whenClosed_isNone() {
        assertEquals(UsbSerialPort.FlowControl.NONE, closedPort().getFlowControl());
    }

    @Test
    public void getWriteEndpoint_isNull() {
        assertNull(closedPort().getWriteEndpoint());
    }

    @Test
    public void getReadEndpoint_whenClosed_isNull() {
        assertNull(closedPort().getReadEndpoint());
    }

    @Test
    public void getSupportedControlLines_listsAllFtdiLines() {
        assertEquals(
                EnumSet.of(UsbSerialPort.ControlLine.RTS, UsbSerialPort.ControlLine.CTS,
                        UsbSerialPort.ControlLine.DTR, UsbSerialPort.ControlLine.DSR,
                        UsbSerialPort.ControlLine.CD, UsbSerialPort.ControlLine.RI),
                closedPort().getSupportedControlLines());
    }

    @Test
    public void getSupportedFlowControl_listsNoneRtsCtsDtrDsrXonXoff() {
        assertEquals(
                EnumSet.of(UsbSerialPort.FlowControl.NONE, UsbSerialPort.FlowControl.RTS_CTS,
                        UsbSerialPort.FlowControl.DTR_DSR, UsbSerialPort.FlowControl.XON_XOFF),
                closedPort().getSupportedFlowControl());
    }

    @Test
    public void setReadQueue_negativeCount_throwsIllegalArgument() {
        assertThrows(IllegalArgumentException.class, () -> closedPort().setReadQueue(-1, 16));
    }

    @Test
    public void setReadQueue_negativeSize_throwsIllegalArgument() {
        assertThrows(IllegalArgumentException.class, () -> closedPort().setReadQueue(2, -1));
    }

    @Test
    public void setReadQueue_validArgs_recordsCountAndSize() {
        final QGCFtdiSerialPort port = closedPort();
        port.setReadQueue(4, 4096);
        assertEquals(4, port.getReadQueueBufferCount());
        assertEquals(4096, port.getReadQueueBufferSize());
    }

    @Test
    public void getPortNumber_isPassthrough() {
        assertTrue(closedPort().getPortNumber() >= 0);
    }
}
