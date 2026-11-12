package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.FakeUsbDeviceConnection;
import android.hardware.usb.FakeUsbEndpoint;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbRequest;
import com.hoho.android.usbserial.driver.CommonUsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import java.io.IOException;
import java.nio.ByteBuffer;

import org.junit.Test;

/**
 * What {@link QGCUsbSerialPort} and the fake driver ports of its tests take for granted about the library's port:
 * how it treats its read queue, and that it refuses transfers once it is closed. Checked on the library's own
 * CommonUsbSerialPort. Only the USB connection under it is a fake, so a version of the library that does
 * otherwise fails here.
 */
public class QGCUsbSerialLibraryContractTest {
    // The read queue that QGCUsbSerialReader sets as it starts reading.
    private static final int QUEUE_BUFFERS = 2;
    private static final int QUEUE_BUFFER_SIZE = 2048;

    /** A request that waits with its connection once it is queued, until the connection hands it out again. */
    private static final class FakeUsbRequest extends UsbRequest {
        private FakeUsbDeviceConnection _connection;
        private Object _clientData;

        @Override
        public boolean initialize(final UsbDeviceConnection connection, final UsbEndpoint endpoint) {
            _connection = (FakeUsbDeviceConnection) connection;
            return true;
        }

        @Override
        public boolean queue(final ByteBuffer buffer, final int length) {
            return _connection.queuedRequests.add(this);
        }

        @Override
        public boolean cancel() {
            return _connection.queuedRequests.remove(this);
        }

        @Override
        public void setClientData(final Object data) {
            _clientData = data;
        }

        @Override
        public Object getClientData() {
            return _clientData;
        }
    }

    /** The library's port with nothing of a chip's driver: it opens on any connection and sends the device nothing. */
    private static final class LibraryPort extends CommonUsbSerialPort {
        LibraryPort() {
            super(new FakeUsbDevice(1002, DEVICE_NAME), 0);
            mUsbRequestSupplier = FakeUsbRequest::new;
        }

        @Override
        protected void openInt() {
            mReadEndpoint = new FakeUsbEndpoint();
            mWriteEndpoint = new FakeUsbEndpoint();
        }

        @Override
        protected void closeInt() {}

        @Override
        public UsbSerialDriver getDriver() {
            return null;
        }

        @Override
        public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) {}
    }

    @Test
    public void read_onAPortWithAReadQueue_takesOnlyABufferOfTheQueuesSizeAndNoTimeout() throws Exception {
        final LibraryPort port = new LibraryPort();
        final FakeUsbDeviceConnection connection = new FakeUsbDeviceConnection();
        port.open(connection);
        port.setReadQueue(QUEUE_BUFFERS, QUEUE_BUFFER_SIZE);
        assertEquals(QUEUE_BUFFERS, connection.queuedRequests.size());

        // As the read thread reads. The transfer that the read took goes back to the device.
        assertEquals(0, port.read(new byte[QUEUE_BUFFER_SIZE], 0));
        assertEquals(QUEUE_BUFFERS, connection.queuedRequests.size());

        assertThrows(IllegalStateException.class, () -> port.read(new byte[QUEUE_BUFFER_SIZE / 2], 0));
        assertThrows(IllegalStateException.class, () -> port.read(new byte[QUEUE_BUFFER_SIZE], 200));
    }

    @Test
    public void read_whoseStatusCheckFails_saysSoWithItsTransferQueuedAgain() throws Exception {
        final LibraryPort port = new LibraryPort();
        final FakeUsbDeviceConnection connection = new FakeUsbDeviceConnection();
        port.open(connection);
        port.setReadQueue(QUEUE_BUFFERS, QUEUE_BUFFER_SIZE);

        // The device does not answer the status request that follows a transfer which came back empty.
        connection.controlTransferResult = -1;
        final IOException failure =
                assertThrows(IOException.class, () -> port.read(new byte[QUEUE_BUFFER_SIZE], 0));

        // What the read thread takes for a failure that it can read on after.
        assertEquals(QGCUsbSerialReader.STATUS_CHECK_FAILED, failure.getMessage());
        assertEquals(QUEUE_BUFFERS, connection.queuedRequests.size());
        connection.controlTransferResult = 0;
        assertEquals(0, port.read(new byte[QUEUE_BUFFER_SIZE], 0));
    }

    @Test
    public void closedPort_refusesTheTransfersOfAThreadThatStillHasIt() throws Exception {
        final LibraryPort port = new LibraryPort();
        port.open(new FakeUsbDeviceConnection());
        port.setReadQueue(QUEUE_BUFFERS, QUEUE_BUFFER_SIZE);
        port.close();

        // What an I/O thread of QGCUsbSerialPort gets that has not ended by the time its port is closed. The
        // driver port is that port's alone, so it stays closed while the next port is open on the device.
        assertThrows(IOException.class, () -> port.read(new byte[QUEUE_BUFFER_SIZE], 0));
        assertThrows(IOException.class, () -> port.write(new byte[16], 16, 0));
    }
}
