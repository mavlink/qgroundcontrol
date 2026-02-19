package org.mavlink.qgroundcontrol;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.IOException;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;

public final class QGCFtdiSerialDriver implements UsbSerialDriver {
    private final UsbDevice _device;
    private final List<UsbSerialPort> _ports;

    public QGCFtdiSerialDriver(final UsbDevice device) {
        _device = device;
        _ports = Collections.singletonList(new QGCFtdiSerialPort(this, device, 0));
    }

    public static Map<Integer, int[]> getSupportedDevices() {
        return Collections.singletonMap(QGCUsbId.VENDOR_FTDI, new int[] {
            QGCUsbId.DEVICE_FTDI_FT232R,
            QGCUsbId.DEVICE_FTDI_FT2232H,
            QGCUsbId.DEVICE_FTDI_FT4232H,
            QGCUsbId.DEVICE_FTDI_FT232H,
            QGCUsbId.DEVICE_FTDI_FT231X
        });
    }

    @Override
    public UsbDevice getDevice() {
        return _device;
    }

    @Override
    public List<UsbSerialPort> getPorts() {
        return _ports;
    }

    private static final class QGCFtdiSerialPort implements UsbSerialPort {
        private final QGCFtdiSerialDriver _driver;
        private final UsbDevice _device;
        private final int _portNumber;

        private UsbDeviceConnection _connection;
        private UsbEndpoint _readEndpoint;
        private UsbEndpoint _writeEndpoint;
        private QGCFtdiDriver _ftdi;
        private int _readQueueBufferCount;
        private int _readQueueBufferSize;

        QGCFtdiSerialPort(final QGCFtdiSerialDriver driver, final UsbDevice device, final int portNumber) {
            _driver = driver;
            _device = device;
            _portNumber = portNumber;
        }

        @Override
        public UsbSerialDriver getDriver() {
            return _driver;
        }

        @Override
        public UsbDevice getDevice() {
            return _device;
        }

        @Override
        public int getPortNumber() {
            return _portNumber;
        }

        @Override
        public UsbEndpoint getWriteEndpoint() {
            return _writeEndpoint;
        }

        @Override
        public UsbEndpoint getReadEndpoint() {
            return _readEndpoint;
        }

        @Override
        public String getSerial() {
            try {
                return _device.getSerialNumber();
            } catch (SecurityException e) {
                return null;
            }
        }

        @Override
        public void setReadQueue(final int bufferCount, final int bufferSize) {
            if (bufferCount < 0) {
                throw new IllegalArgumentException("Invalid bufferCount");
            }
            if (bufferSize < 0) {
                throw new IllegalArgumentException("Invalid bufferSize");
            }

            int effectiveBufferSize = bufferSize;
            if (isOpen() && (effectiveBufferSize == 0) && (_readEndpoint != null)) {
                effectiveBufferSize = _readEndpoint.getMaxPacketSize();
            }

            if (isOpen()) {
                if (bufferCount < _readQueueBufferCount) {
                    throw new IllegalStateException("Cannot reduce bufferCount when port is open");
                }
                if ((_readQueueBufferCount != 0) && (_readQueueBufferSize != 0) && (effectiveBufferSize != _readQueueBufferSize)) {
                    throw new IllegalStateException("Cannot change bufferSize when port is open");
                }
            }

            _readQueueBufferCount = bufferCount;
            _readQueueBufferSize = effectiveBufferSize;
        }

        @Override
        public int getReadQueueBufferCount() {
            return _readQueueBufferCount;
        }

        @Override
        public int getReadQueueBufferSize() {
            return _readQueueBufferSize;
        }

        @Override
        public void open(final UsbDeviceConnection connection) throws IOException {
            if (isOpen()) {
                throw new IOException("Already open");
            }
            if (connection == null) {
                throw new IOException("Null USB device connection");
            }

            final QGCFtdiDriver ftdi = QGCFtdiDriver.open(_device);
            if (ftdi == null || !ftdi.isOpen()) {
                throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName());
            }

            _connection = connection;
            _ftdi = ftdi;

            if (!resolveBulkEndpoints()) {
                close();
                throw new IOException("Failed to resolve FTDI bulk endpoints for " + _device.getDeviceName());
            }
        }

        @Override
        public void close() throws IOException {
            if (_ftdi != null) {
                _ftdi.close();
                _ftdi = null;
            }

            if (_connection != null) {
                _connection.close();
                _connection = null;
            }

            _readEndpoint = null;
            _writeEndpoint = null;
        }

        @Override
        public int read(final byte[] dest, final int timeout) throws IOException {
            return read(dest, dest.length, timeout);
        }

        @Override
        public int read(final byte[] dest, final int length, final int timeout) throws IOException {
            ensureOpen();
            if (dest == null) {
                throw new IOException("Null read buffer");
            }
            if (length <= 0) {
                return 0;
            }

            final int readLength = Math.min(length, dest.length);
            final byte[] data = _ftdi.read(readLength, timeout);
            if (data.length > 0) {
                System.arraycopy(data, 0, dest, 0, data.length);
            }
            return data.length;
        }

        @Override
        public void write(final byte[] src, final int timeout) throws IOException {
            write(src, src.length, timeout);
        }

        @Override
        public void write(final byte[] src, final int length, final int timeout) throws IOException {
            ensureOpen();
            if (src == null) {
                throw new IOException("Null write buffer");
            }
            if (length <= 0) {
                return;
            }

            final int writeLength = Math.min(length, src.length);
            final int written = _ftdi.write(src, writeLength, timeout);
            if (written < writeLength) {
                throw new IOException("D2XX write failed or short write: " + written + " / " + writeLength);
            }
        }

        @Override
        public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) throws IOException {
            ensureOpen();
            if (!_ftdi.setParameters(baudRate, dataBits, stopBits, parity)) {
                throw new IOException("Failed to set FTDI serial parameters");
            }
        }

        @Override
        public boolean getCD() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.CD);
        }

        @Override
        public boolean getCTS() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.CTS);
        }

        @Override
        public boolean getDSR() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.DSR);
        }

        @Override
        public boolean getDTR() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.DTR);
        }

        @Override
        public void setDTR(final boolean value) throws IOException {
            ensureOpen();
            if (!_ftdi.setControlLine(ControlLine.DTR, value)) {
                throw new IOException("Failed to set DTR");
            }
        }

        @Override
        public boolean getRI() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.RI);
        }

        @Override
        public boolean getRTS() throws IOException {
            ensureOpen();
            return _ftdi.getControlLine(ControlLine.RTS);
        }

        @Override
        public void setRTS(final boolean value) throws IOException {
            ensureOpen();
            if (!_ftdi.setControlLine(ControlLine.RTS, value)) {
                throw new IOException("Failed to set RTS");
            }
        }

        @Override
        public EnumSet<ControlLine> getControlLines() throws IOException {
            ensureOpen();
            final EnumSet<ControlLine> lines = EnumSet.noneOf(ControlLine.class);
            if (getRTS()) {
                lines.add(ControlLine.RTS);
            }
            if (getCTS()) {
                lines.add(ControlLine.CTS);
            }
            if (getDTR()) {
                lines.add(ControlLine.DTR);
            }
            if (getDSR()) {
                lines.add(ControlLine.DSR);
            }
            if (getCD()) {
                lines.add(ControlLine.CD);
            }
            if (getRI()) {
                lines.add(ControlLine.RI);
            }
            return lines;
        }

        @Override
        public EnumSet<ControlLine> getSupportedControlLines() {
            return EnumSet.of(ControlLine.RTS, ControlLine.CTS, ControlLine.DTR, ControlLine.DSR, ControlLine.CD, ControlLine.RI);
        }

        @Override
        public void setFlowControl(final FlowControl flowControl) throws IOException {
            ensureOpen();
            if (flowControl == FlowControl.XON_XOFF_INLINE) {
                throw new IOException("XON/XOFF inline flow control is not supported by D2XX adapter");
            }

            if (!_ftdi.setFlowControl(flowControl.ordinal())) {
                throw new IOException("Failed to set flow control: " + flowControl);
            }
        }

        @Override
        public FlowControl getFlowControl() {
            if (!isOpen()) {
                return FlowControl.NONE;
            }
            final int ordinal = _ftdi.getFlowControl();
            final FlowControl[] values = FlowControl.values();
            if (ordinal < 0 || ordinal >= values.length) {
                return FlowControl.NONE;
            }
            return values[ordinal];
        }

        @Override
        public EnumSet<FlowControl> getSupportedFlowControl() {
            return EnumSet.of(FlowControl.NONE, FlowControl.RTS_CTS, FlowControl.DTR_DSR, FlowControl.XON_XOFF);
        }

        @Override
        public boolean getXON() throws IOException {
            ensureOpen();
            return false;
        }

        @Override
        public void purgeHwBuffers(final boolean purgeReadBuffers, final boolean purgeWriteBuffers) throws IOException {
            ensureOpen();
            if (!_ftdi.purgeBuffers(purgeReadBuffers, purgeWriteBuffers)) {
                throw new IOException("Failed to purge FTDI buffers");
            }
        }

        @Override
        public void setBreak(final boolean value) throws IOException {
            ensureOpen();
            if (!_ftdi.setBreak(value)) {
                throw new IOException("Failed to set break condition");
            }
        }

        @Override
        public boolean isOpen() {
            return _ftdi != null && _ftdi.isOpen();
        }

        private void ensureOpen() throws IOException {
            if (!isOpen()) {
                throw new IOException("Port not open");
            }
        }

        private boolean resolveBulkEndpoints() {
            UsbEndpoint read = null;
            UsbEndpoint write = null;

            for (int i = 0; i < _device.getInterfaceCount(); i++) {
                final UsbInterface usbInterface = _device.getInterface(i);
                for (int e = 0; e < usbInterface.getEndpointCount(); e++) {
                    final UsbEndpoint endpoint = usbInterface.getEndpoint(e);
                    if (endpoint.getType() != UsbConstants.USB_ENDPOINT_XFER_BULK) {
                        continue;
                    }
                    if (endpoint.getDirection() == UsbConstants.USB_DIR_IN && read == null) {
                        read = endpoint;
                    } else if (endpoint.getDirection() == UsbConstants.USB_DIR_OUT && write == null) {
                        write = endpoint;
                    }
                }
            }

            _readEndpoint = read;
            _writeEndpoint = write;

            return _readEndpoint != null && _writeEndpoint != null;
        }
    }
}
