package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;

public final class QGCFtdiSerialDriver implements UsbSerialDriver {

    private static final String TAG = QGCFtdiSerialDriver.class.getSimpleName();

    // D2XX manager lifecycle — shared across all ports on all FTDI devices.
    private static volatile Context sAppContext;
    private static volatile D2xxManager sManager;

    private final UsbDevice _device;
    private final List<UsbSerialPort> _ports;

    public QGCFtdiSerialDriver(final UsbDevice device) {
        _device = device;

        final int interfaceCount = device.getInterfaceCount();
        if (interfaceCount <= 0) {
            _ports = Collections.emptyList();
            return;
        }
        final List<UsbSerialPort> ports = new ArrayList<>(interfaceCount);
        for (int i = 0; i < interfaceCount; i++) {
            ports.add(new QGCFtdiSerialPort(this, device, i));
        }

        _ports = Collections.unmodifiableList(ports);
    }

    // -------------------------------------------------------------------------
    // D2XX lifecycle
    // -------------------------------------------------------------------------

    public static void initialize(final Context context) {
        sAppContext = context.getApplicationContext();
        try {
            sManager = D2xxManager.getInstance(sAppContext);
        } catch (D2xxManager.D2xxException e) {
            sManager = null;
            QGCLogger.w(TAG, "D2XX manager unavailable: " + e.getMessage());
        } catch (Throwable t) {
            sManager = null;
            QGCLogger.w(TAG, "D2XX manager unavailable: " + t.getMessage());
        }
    }

    public static void cleanup() {
        sManager = null;
        sAppContext = null;
    }

    public static boolean isAvailable() {
        return (sManager != null) && (sAppContext != null);
    }

    public static boolean isFtdiDevice(final UsbDevice device) {
        return device != null
            && device.getVendorId() == QGCUsbId.VENDOR_FTDI
            && QGCUsbId.isSupportedFtdiProductId(device.getProductId());
    }

    // -------------------------------------------------------------------------
    // UsbSerialDriver
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // D2XX conversion helpers
    // -------------------------------------------------------------------------

    private static byte toD2xxDataBits(final int dataBits) {
        return (dataBits == 7) ? D2xxManager.FT_DATA_BITS_7 : D2xxManager.FT_DATA_BITS_8;
    }

    private static byte toD2xxStopBits(final int stopBits) {
        if (stopBits == UsbSerialPort.STOPBITS_2) {
            return D2xxManager.FT_STOP_BITS_2;
        }
        return D2xxManager.FT_STOP_BITS_1;
    }

    private static byte toD2xxParity(final int parity) {
        switch (parity) {
            case UsbSerialPort.PARITY_ODD:
                return D2xxManager.FT_PARITY_ODD;
            case UsbSerialPort.PARITY_EVEN:
                return D2xxManager.FT_PARITY_EVEN;
            case UsbSerialPort.PARITY_MARK:
                return D2xxManager.FT_PARITY_MARK;
            case UsbSerialPort.PARITY_SPACE:
                return D2xxManager.FT_PARITY_SPACE;
            case UsbSerialPort.PARITY_NONE:
            default:
                return D2xxManager.FT_PARITY_NONE;
        }
    }

    private static short toD2xxFlowControl(final int flowControl) {
        if (flowControl == UsbSerialPort.FlowControl.RTS_CTS.ordinal()) {
            return D2xxManager.FT_FLOW_RTS_CTS;
        }
        if (flowControl == UsbSerialPort.FlowControl.DTR_DSR.ordinal()) {
            return D2xxManager.FT_FLOW_DTR_DSR;
        }
        if (flowControl == UsbSerialPort.FlowControl.XON_XOFF.ordinal()) {
            return D2xxManager.FT_FLOW_XON_XOFF;
        }
        return D2xxManager.FT_FLOW_NONE;
    }

    // -------------------------------------------------------------------------
    // Inner port class
    // -------------------------------------------------------------------------

    private static final class QGCFtdiSerialPort implements UsbSerialPort {
        private final QGCFtdiSerialDriver _driver;
        private final UsbDevice _device;
        private final int _portNumber;

        private UsbDeviceConnection _connection;
        private UsbEndpoint _readEndpoint;
        private UsbEndpoint _writeEndpoint;
        private FT_Device _ftDevice;
        private int _flowControl = UsbSerialPort.FlowControl.NONE.ordinal();
        private boolean _dtr;
        private boolean _rts;
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
            if (sManager == null || sAppContext == null || !isFtdiDevice(_device)) {
                throw new IOException("D2XX manager unavailable or device is not an FTDI device");
            }

            FT_Device d2xxDevice;
            try {
                d2xxDevice = sManager.openByUsbDevice(sAppContext, _device);
            } catch (Throwable t) {
                throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName() + ": " + t.getMessage());
            }

            if (d2xxDevice == null || !d2xxDevice.isOpen()) {
                throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName());
            }

            _connection = connection;
            _ftDevice = d2xxDevice;

            if (!resolveBulkEndpoints()) {
                // Close only the D2XX handle; do not close the caller-provided
                // connection — the caller owns its lifecycle.
                try {
                    _ftDevice.close();
                } catch (Throwable ignored) {}
                _ftDevice = null;
                _connection = null;
                _readEndpoint = null;
                _writeEndpoint = null;
                throw new IOException("Failed to resolve FTDI bulk endpoints for " + _device.getDeviceName());
            }
        }

        @Override
        public void close() throws IOException {
            if (_ftDevice != null) {
                try {
                    _ftDevice.close();
                } catch (Throwable t) {
                    QGCLogger.w(TAG, "Error closing D2XX device: " + t.getMessage());
                }
                _ftDevice = null;
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
            final byte[] buffer = new byte[readLength];
            try {
                final int bytesRead = _ftDevice.read(buffer, readLength, timeout);
                if (bytesRead <= 0) {
                    return 0;
                }
                final int n = Math.min(bytesRead, readLength);
                System.arraycopy(buffer, 0, dest, 0, n);
                return n;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error reading D2XX data", t);
                return 0;
            }
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
            final byte[] writeData = (writeLength == src.length) ? src : Arrays.copyOf(src, writeLength);
            try {
                final int written = _ftDevice.write(writeData, writeLength, true, timeout);
                if (written < writeLength) {
                    throw new IOException("D2XX write failed or short write: " + written + " / " + writeLength);
                }
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error writing D2XX data", t);
                throw new IOException("D2XX write error: " + t.getMessage());
            }
        }

        @Override
        public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) throws IOException {
            ensureOpen();
            try {
                final boolean baudOk = _ftDevice.setBaudRate(baudRate);
                final boolean charsOk = _ftDevice.setDataCharacteristics(
                    toD2xxDataBits(dataBits), toD2xxStopBits(stopBits), toD2xxParity(parity));
                if (!baudOk || !charsOk) {
                    throw new IOException("Failed to set FTDI serial parameters");
                }
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error setting D2XX parameters", t);
                throw new IOException("Failed to set FTDI serial parameters: " + t.getMessage());
            }
        }

        @Override
        public boolean getCD() throws IOException {
            ensureOpen();
            return readModemStatusBit(D2xxManager.FT_DCD);
        }

        @Override
        public boolean getCTS() throws IOException {
            ensureOpen();
            return readModemStatusBit(D2xxManager.FT_CTS);
        }

        @Override
        public boolean getDSR() throws IOException {
            ensureOpen();
            return readModemStatusBit(D2xxManager.FT_DSR);
        }

        @Override
        public boolean getDTR() throws IOException {
            ensureOpen();
            return _dtr;
        }

        @Override
        public void setDTR(final boolean value) throws IOException {
            ensureOpen();
            try {
                final boolean ok = value ? _ftDevice.setDtr() : _ftDevice.clrDtr();
                if (!ok) {
                    throw new IOException("Failed to set DTR");
                }
                _dtr = value;
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error setting D2XX control line DTR", t);
                throw new IOException("Failed to set DTR: " + t.getMessage());
            }
        }

        @Override
        public boolean getRI() throws IOException {
            ensureOpen();
            return readModemStatusBit(D2xxManager.FT_RI);
        }

        @Override
        public boolean getRTS() throws IOException {
            ensureOpen();
            return _rts;
        }

        @Override
        public void setRTS(final boolean value) throws IOException {
            ensureOpen();
            try {
                final boolean ok = value ? _ftDevice.setRts() : _ftDevice.clrRts();
                if (!ok) {
                    throw new IOException("Failed to set RTS");
                }
                _rts = value;
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error setting D2XX control line RTS", t);
                throw new IOException("Failed to set RTS: " + t.getMessage());
            }
        }

        @Override
        public EnumSet<ControlLine> getControlLines() throws IOException {
            ensureOpen();
            final EnumSet<ControlLine> lines = EnumSet.noneOf(ControlLine.class);
            if (getRTS()) lines.add(ControlLine.RTS);
            if (getCTS()) lines.add(ControlLine.CTS);
            if (getDTR()) lines.add(ControlLine.DTR);
            if (getDSR()) lines.add(ControlLine.DSR);
            if (getCD())  lines.add(ControlLine.CD);
            if (getRI())  lines.add(ControlLine.RI);
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
            try {
                final byte XON  = 0x11; // DC1 — standard ASCII XON
                final byte XOFF = 0x13; // DC3 — standard ASCII XOFF
                final short flowMode = toD2xxFlowControl(flowControl.ordinal());
                final boolean ok = _ftDevice.setFlowControl(flowMode, XON, XOFF);
                if (!ok) {
                    throw new IOException("Failed to set flow control: " + flowControl);
                }
                _flowControl = flowControl.ordinal();
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error setting D2XX flow control", t);
                throw new IOException("Failed to set flow control: " + t.getMessage());
            }
        }

        @Override
        public FlowControl getFlowControl() {
            if (!isOpen()) {
                return FlowControl.NONE;
            }
            final FlowControl[] values = FlowControl.values();
            if (_flowControl < 0 || _flowControl >= values.length) {
                return FlowControl.NONE;
            }
            return values[_flowControl];
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
            byte purgeFlags = 0;
            if (purgeReadBuffers)  purgeFlags |= D2xxManager.FT_PURGE_RX;
            if (purgeWriteBuffers) purgeFlags |= D2xxManager.FT_PURGE_TX;
            if (purgeFlags == 0) {
                return;
            }
            try {
                if (!_ftDevice.purge(purgeFlags)) {
                    throw new IOException("Failed to purge FTDI buffers");
                }
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error purging D2XX buffers", t);
                throw new IOException("Failed to purge FTDI buffers: " + t.getMessage());
            }
        }

        @Override
        public void setBreak(final boolean value) throws IOException {
            ensureOpen();
            try {
                final boolean ok = value ? _ftDevice.setBreakOn() : _ftDevice.setBreakOff();
                if (!ok) {
                    throw new IOException("Failed to set break condition");
                }
            } catch (IOException e) {
                throw e;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error setting D2XX break condition", t);
                throw new IOException("Failed to set break condition: " + t.getMessage());
            }
        }

        @Override
        public boolean isOpen() {
            return _ftDevice != null && _ftDevice.isOpen();
        }

        private void ensureOpen() throws IOException {
            if (!isOpen()) {
                throw new IOException("Port not open");
            }
        }

        private boolean readModemStatusBit(final int mask) {
            try {
                return (_ftDevice.getModemStatus() & mask) != 0;
            } catch (Throwable t) {
                QGCLogger.e(TAG, "Error reading D2XX modem status", t);
                return false;
            }
        }

        private boolean resolveBulkEndpoints() {
            UsbEndpoint read = null;
            UsbEndpoint write = null;

            if (_portNumber < _device.getInterfaceCount()) {
                final UsbInterface usbInterface = _device.getInterface(_portNumber);
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

            // Fallback: scan interface 0 only for single-port devices that
            // don't expose interface-per-port semantics. For multi-port devices,
            // scanning all interfaces would incorrectly assign port 0's endpoints
            // to every port.
            if ((read == null || write == null) && _device.getInterfaceCount() == 1) {
                final UsbInterface usbInterface = _device.getInterface(0);
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
