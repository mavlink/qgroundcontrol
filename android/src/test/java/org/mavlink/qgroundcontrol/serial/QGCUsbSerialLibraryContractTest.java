package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.awaitTrue;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.FakeUsbDeviceConnection;
import android.hardware.usb.FakeUsbEndpoint;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbRequest;
import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.CommonUsbSerialPort;
import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;

import org.junit.After;
import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.FakeUsbHost;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.RecordingCallbacks;

/**
 * What {@link QGCUsbSerialPort} and the fake driver ports of its tests take for granted about the library's port:
 * how it treats its read queue, how it splits a write into USB transfers, and that it refuses transfers once it
 * is closed. Checked on the library's own CommonUsbSerialPort. Only the USB connection under it is a fake, so a
 * version of the library that does otherwise fails here. Also that the library's constants for data bits, stop
 * bits and parity are the numbers that C++ sends.
 */
public class QGCUsbSerialLibraryContractTest {
    // The read queue that QGCUsbSerialReader sets as it starts reading.
    private static final int QUEUE_BUFFERS = 2;
    private static final int QUEUE_BUFFER_SIZE = 2048;
    private static final int PIECE_BYTES = QGCUsbSerialWriter.PIECE_BYTES;
    private static final int PACKET_BYTES = new FakeUsbEndpoint().getMaxPacketSize();
    // Of the port that a test has the app open. The ID is far from those of the ports of the other tests, which
    // find a port's I/O threads by the ID in their names.
    private static final int APP_PORT_ID = 1_000_000;
    private static final long HANDLE = 7;
    // More than a test takes, so that no write runs out of time.
    private static final int WRITE_TIMEOUT_MS = 60_000;

    private QGCUsbSerialPort _appPort;

    @After
    public void tearDown() {
        // Ends the port's write thread, also for a test that failed.
        if (_appPort != null) {
            _appPort.close();
        }
    }

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
    public void driverPort_tellsItsFlowControlsWithoutBeingOpened() {
        // What QGCUsbSerialDevices asks of a port as it lists a device, which may be one that QGC may not open.
        final FakeUsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        assertEquals(EnumSet.of(UsbSerialPort.FlowControl.NONE),
                new CdcAcmSerialDriver(board).getPorts().get(0).getSupportedFlowControl());
        final FakeUsbDevice adapter = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        // The FTDI driver makes a port for each interface of its device.
        adapter.interfaceCount = 1;
        assertTrue(new FtdiSerialDriver(adapter).getPorts().get(0).getSupportedFlowControl()
                .contains(UsbSerialPort.FlowControl.RTS_CTS));
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

    @Test
    public void write_onAPortAsTheLibraryMakesIt_sendsAPacketToATransfer() throws Exception {
        final LibraryPort port = new LibraryPort();
        final FakeUsbDeviceConnection connection = new FakeUsbDeviceConnection();
        port.open(connection);

        port.write(new byte[PIECE_BYTES], PIECE_BYTES, 0);

        // Why QGCUsbSerialPort sets the library's port up as it opens it: a piece would take this many transfers.
        assertEquals(Collections.nCopies(PIECE_BYTES / PACKET_BYTES, PACKET_BYTES), connection.bulkTransferLengths);
    }

    @Test
    public void writeAsync_onAPortThatTheAppOpened_sendsAPieceAsOneTransfer() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final FakeUsbDeviceConnection connection = openAsTheApp(callbacks);
        final int length = PIECE_BYTES + 300;

        assertEquals(length, _appPort.writeAsync(new byte[length], length));
        awaitTrue(() -> !callbacks.events.isEmpty());

        assertEquals(List.of("written 7 " + length), callbacks.events);
        assertEquals(List.of(PIECE_BYTES, 300), connection.bulkTransferLengths);
    }

    @Test
    public void writeWithProgress_withATimeout_sendsAPacketToATransfer() throws Exception {
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final FakeUsbDeviceConnection connection = openAsTheApp(callbacks);
        final int length = 3 * PACKET_BYTES + 8;

        // A transfer that ran out of time would not tell how much of it the device took. Of a packet that is all
        // or nothing, so QGCUsbSerialPort can tell how much of the write went out.
        assertEquals(length, _appPort.writeWithProgress(new byte[length], length, WRITE_TIMEOUT_MS));
        assertEquals(List.of(PACKET_BYTES, PACKET_BYTES, PACKET_BYTES, 8), connection.bulkTransferLengths);

        // The write thread's pieces go out whole again.
        assertEquals(PIECE_BYTES, _appPort.writeAsync(new byte[PIECE_BYTES], PIECE_BYTES));
        awaitTrue(() -> !callbacks.events.isEmpty());
        assertEquals(List.of(PACKET_BYTES, PACKET_BYTES, PACKET_BYTES, 8, PIECE_BYTES),
                     connection.bulkTransferLengths);
    }

    @Test
    public void dataBitsStopBitsAndParity_haveTheNumbersThatCppSends() {
        // toJavaDataBits(), toJavaStopBits() and toJavaParity() in AndroidSerialJni.cc send these numbers, and
        // QGCUsbSerialPort hands them to the library as they are. test_android_serial_jni_contract.py reads
        // the numbers from these lines and compares them with that file, so keep a number and its constant
        // in one assertEquals.
        assertEquals(5, UsbSerialPort.DATABITS_5);
        assertEquals(6, UsbSerialPort.DATABITS_6);
        assertEquals(7, UsbSerialPort.DATABITS_7);
        assertEquals(8, UsbSerialPort.DATABITS_8);
        assertEquals(1, UsbSerialPort.STOPBITS_1);
        assertEquals(3, UsbSerialPort.STOPBITS_1_5);
        assertEquals(2, UsbSerialPort.STOPBITS_2);
        assertEquals(0, UsbSerialPort.PARITY_NONE);
        assertEquals(1, UsbSerialPort.PARITY_ODD);
        assertEquals(2, UsbSerialPort.PARITY_EVEN);
        assertEquals(3, UsbSerialPort.PARITY_MARK);
        assertEquals(4, UsbSerialPort.PARITY_SPACE);
    }

    /**
     * Has the app open the library's port, as {@link QGCUsbSerialDevices} does for a port that C++ opens. Returns
     * the USB connection under it.
     */
    private FakeUsbDeviceConnection openAsTheApp(final QGCUsbSerialCallbacks callbacks) {
        final FakeUsbHost usbHost = new FakeUsbHost();
        _appPort = new QGCUsbSerialPort(APP_PORT_ID, HANDLE, new QGCUsbSerialDevices.PortAddress(1, 0),
                new LibraryPort(), callbacks);
        assertEquals(QGCUsbSerialResult.OK, _appPort.open(usbHost));
        return usbHost.connections.get(0);
    }
}
