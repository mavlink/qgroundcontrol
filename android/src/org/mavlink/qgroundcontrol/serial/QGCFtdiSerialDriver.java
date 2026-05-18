package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

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
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

public final class QGCFtdiSerialDriver implements UsbSerialDriver {

    private static final String TAG = QGCFtdiSerialDriver.class.getSimpleName();

    // FTDI VID + the PIDs this D2XX driver claims. usb-serial-for-android's stock
    // FtdiSerialDriver also handles these PIDs in VCP mode; the prober swaps to this
    // class when D2XX is available so devices in D2XX-mode firmware work.
    private static final int VENDOR_FTDI         = 0x0403;
    private static final int DEVICE_FTDI_FT232R  = 0x6001;
    private static final int DEVICE_FTDI_FT2232H = 0x6010;
    private static final int DEVICE_FTDI_FT4232H = 0x6011;
    private static final int DEVICE_FTDI_FT232H  = 0x6014;
    private static final int DEVICE_FTDI_FT231X  = 0x6015;

    // D2XX is OFF by default. Regression: on Android 14 / OneUI 6 (QGC issue #14146),
    // D2XX.createDeviceInfoList() enumerates the device and serial matches succeed,
    // but D2xxManager.openByUsbDevice() then fails — so canOpenViaD2XX() reports the
    // device as openable when it isn't, and the prober commits to a driver that
    // cannot open. VCP-mode FtdiSerialDriver works for the same chips and already
    // gets the 1ms latency-timer drop via vendor control transfer in
    // UsbSerialLifecycle.openDriver (FTDI_SIO_SET_LATENCY). Re-enable for hardware
    // validation, not for shipping.
    public static final boolean ENABLE_D2XX = false;

    /**
     * D2XX manager lifecycle — shared across all ports on all FTDI devices.
     * The manager and its app Context are published together via a single
     * {@link AtomicReference} so open() sees a consistent (manager, context)
     * pair even if cleanup() races in from another thread.
     */
    private static final class D2xxHandle {
        final Context appContext;
        final D2xxManager manager;
        D2xxHandle(final Context appContext, final D2xxManager manager) {
            this.appContext = appContext;
            this.manager = manager;
        }
    }

    private static final AtomicReference<D2xxHandle> sD2xx = new AtomicReference<>();

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

    public static void initialize(final Context context) {
        final Context appContext = context.getApplicationContext();
        D2xxManager manager = null;
        try {
            manager = D2xxManager.getInstance(appContext);
        } catch (D2xxManager.D2xxException e) {
            QGCLogger.w(TAG, "D2XX manager unavailable: " + e.getMessage());
        } catch (Throwable t) {
            QGCLogger.w(TAG, "D2XX manager unavailable: " + t.getMessage());
        }
        sD2xx.set((manager != null) ? new D2xxHandle(appContext, manager) : null);
    }

    public static void cleanup() {
        sD2xx.set(null);
    }

    public static boolean isAvailable() {
        return sD2xx.get() != null;
    }

    public static boolean isFtdiDevice(final UsbDevice device) {
        if (device == null || device.getVendorId() != VENDOR_FTDI) return false;
        final int pid = device.getProductId();
        return pid == DEVICE_FTDI_FT232R || pid == DEVICE_FTDI_FT2232H
            || pid == DEVICE_FTDI_FT4232H || pid == DEVICE_FTDI_FT232H
            || pid == DEVICE_FTDI_FT231X;
    }

    public static Map<Integer, int[]> getSupportedDevices() {
        return Collections.singletonMap(VENDOR_FTDI, new int[] {
            DEVICE_FTDI_FT232R,
            DEVICE_FTDI_FT2232H,
            DEVICE_FTDI_FT4232H,
            DEVICE_FTDI_FT232H,
            DEVICE_FTDI_FT231X,
        });
    }

    /**
     * Returns true only if the D2XX library currently enumerates {@code device}.
     * Generic FT232R USB-TTL adapters present in VCP mode and are not enumerated
     * by D2XX — those must fall through to the stock {@code FtdiSerialDriver}.
     */
    public static boolean canOpenViaD2XX(final UsbDevice device) {
        if (!isFtdiDevice(device)) return false;
        final D2xxHandle h = sD2xx.get();
        if (h == null) return false;
        try {
            final int count = h.manager.createDeviceInfoList(h.appContext);
            if (count <= 0) return false;
            final D2xxManager.FtDeviceInfoListNode[] nodes = new D2xxManager.FtDeviceInfoListNode[count];
            final int got = h.manager.getDeviceInfoList(count, nodes);
            if (got <= 0) return false;
            final String deviceSerial = device.getSerialNumber();
            for (int i = 0; i < got; i++) {
                final D2xxManager.FtDeviceInfoListNode node = nodes[i];
                if (node == null) continue;
                if (deviceSerial != null && deviceSerial.equals(node.serialNumber)) return true;
            }
        } catch (Throwable t) {
            QGCLogger.w(TAG, "canOpenViaD2XX check failed: " + t.getMessage());
        }
        return false;
    }

    @Override
    public UsbDevice getDevice() {
        return _device;
    }

    @Override
    public List<UsbSerialPort> getPorts() {
        return _ports;
    }

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

    private static short toD2xxFlowControl(final UsbSerialPort.FlowControl flowControl) {
        if (flowControl == UsbSerialPort.FlowControl.RTS_CTS) {
            return D2xxManager.FT_FLOW_RTS_CTS;
        }
        if (flowControl == UsbSerialPort.FlowControl.DTR_DSR) {
            return D2xxManager.FT_FLOW_DTR_DSR;
        }
        if (flowControl == UsbSerialPort.FlowControl.XON_XOFF) {
            return D2xxManager.FT_FLOW_XON_XOFF;
        }
        return D2xxManager.FT_FLOW_NONE;
    }

    /** Package-visible for {@code UsbSerialLifecycle}'s baud-listener wiring. */
    static final class QGCFtdiSerialPort implements UsbSerialPort {
        private final QGCFtdiSerialDriver _driver;
        private final UsbDevice _device;
        private final int _portNumber;

        private UsbDeviceConnection _connection;
        private FT_Device _ftDevice;
        private FlowControl _flowControl = FlowControl.NONE;
        private boolean _dtr;
        /** Current configured baud — exposed for the write pump's wire-rate throttle. */
        private volatile int _currentBaudRate = 9600;
        private volatile java.util.function.IntConsumer _baudListener;
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

        // D2XX library performs I/O internally; the bulk endpoints are exposed only so the
        // I/O manager can size its read buffer via getMaxPacketSize(). We resolve lazily on
        // demand and never fail port open() over an unresolved endpoint.
        @Override
        public UsbEndpoint getWriteEndpoint() {
            return null;
        }

        @Override
        public UsbEndpoint getReadEndpoint() {
            if (!isOpen()) {
                return null;
            }
            final int interfaceCount = _device.getInterfaceCount();
            final int interfaceIndex = (_portNumber < interfaceCount) ? _portNumber
                    : (interfaceCount == 1 ? 0 : -1);
            if (interfaceIndex < 0) {
                return null;
            }
            final UsbInterface usbInterface = _device.getInterface(interfaceIndex);
            for (int e = 0; e < usbInterface.getEndpointCount(); e++) {
                final UsbEndpoint endpoint = usbInterface.getEndpoint(e);
                if (endpoint.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK
                        && endpoint.getDirection() == UsbConstants.USB_DIR_IN) {
                    return endpoint;
                }
            }
            return null;
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
            if (bufferCount < 0 || bufferSize < 0) {
                throw new IllegalArgumentException("setReadQueue: negative arguments");
            }
            // D2XX manages its own read buffering; these values are stored for reflection only.
            _readQueueBufferCount = bufferCount;
            _readQueueBufferSize = bufferSize;
        }

        @Override
        public int getReadQueueBufferCount() {
            return _readQueueBufferCount;
        }

        @Override
        public int getReadQueueBufferSize() {
            return _readQueueBufferSize;
        }

        @FunctionalInterface
        private interface D2xxOp<T> { T run() throws IOException; }

        private <T> T d2xxOp(final String desc, final D2xxOp<T> op) throws IOException {
            try { return op.run(); }
            catch (final IOException e) { throw e; }
            catch (final Throwable t) {
                QGCLogger.e(TAG, "Error " + desc, t);
                throw new IOException(desc + " failed: " + t.getMessage(), t);
            }
        }

        @Override
        public void open(final UsbDeviceConnection connection) throws IOException {
            if (isOpen()) {
                throw new IOException("Already open");
            }
            if (connection == null) {
                throw new IOException("Null USB device connection");
            }
            // Snapshot the (manager, context) pair atomically so cleanup() on another
            // thread cannot null one of them between this check and the openByUsbDevice
            // call below.
            final D2xxHandle handle = sD2xx.get();
            if (handle == null || !isFtdiDevice(_device)) {
                throw new IOException("D2XX manager unavailable or device is not an FTDI device");
            }

            FT_Device d2xxDevice;
            try {
                d2xxDevice = handle.manager.openByUsbDevice(handle.appContext, _device);
            } catch (Throwable t) {
                throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName() + ": " + t.getMessage());
            }

            if (d2xxDevice == null || !d2xxDevice.isOpen()) {
                throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName());
            }

            _connection = connection;
            _ftDevice = d2xxDevice;

            // FTDI default latency timer is 16ms, which caps short-read flush rate at ~62 Hz —
            // bad for MAVLink telemetry. 1ms is the minimum the chip accepts. Best-effort.
            try {
                _ftDevice.setLatencyTimer((byte) 1);
            } catch (Throwable t) {
                QGCLogger.w(TAG, "setLatencyTimer failed: " + t.getMessage());
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
            try {
                final int bytesRead = _ftDevice.read(dest, readLength, timeout);
                return Math.max(0, Math.min(bytesRead, readLength));
            } catch (Throwable t) {
                // Hot-unplug surfaces from D2XX as RuntimeException / NullPointerException
                // out of JNI (the lib doesn't declare IOException). Returning 0 here
                // would loop SerialInputOutputManager forever without firing onRunError,
                // so the C++ side never sees EXC_RESOURCE and the port stays "open" from
                // its perspective. Throw so the IO manager exits and the listener-mute
                // / token-stale path runs.
                QGCLogger.e(TAG, "Error reading D2XX data", t);
                throw new IOException("D2XX read error: " + t.getMessage(), t);
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
            d2xxOp("writing D2XX data", () -> {
                // wait=true is required: FT_Device reuses a single UsbRequest per device and
                // wait=false returns before the URB completes, so the next write throws
                // IllegalStateException("this request is currently queued"). The cost is that
                // each write blocks the JNI write thread until the FTDI chip physically clocks
                // bytes out — fine at high baud, slow at low baud. This is why FT232R/FT232H
                // USB-TTL adapters wired to low-baud TELEM ports are routed through mik3y by
                // QGCUsbSerialProber instead.
                final int written = _ftDevice.write(src, writeLength, true, timeout);
                if (written < writeLength) {
                    throw new IOException("D2XX write failed or short write: " + written + " / " + writeLength);
                }
                return null;
            });
        }

        @Override
        public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) throws IOException {
            ensureOpen();
            d2xxOp("setting D2XX parameters", () -> {
                final boolean baudOk = _ftDevice.setBaudRate(baudRate);
                final boolean charsOk = _ftDevice.setDataCharacteristics(
                    toD2xxDataBits(dataBits), toD2xxStopBits(stopBits), toD2xxParity(parity));
                if (!baudOk || !charsOk) {
                    throw new IOException("Failed to set FTDI serial parameters");
                }
                return null;
            });
            _currentBaudRate = baudRate;
            final java.util.function.IntConsumer l = _baudListener;
            if (l != null) l.accept(baudRate);
        }

        /** Current baud configured on the chip (last successful setParameters). */
        public int getCurrentBaudRate() {
            return _currentBaudRate;
        }

        /** Registers a listener invoked after every successful {@link #setParameters} call.
         *  Fires once immediately so a listener wired post-open picks up the current baud. */
        public void setBaudListener(final java.util.function.IntConsumer listener) {
            _baudListener = listener;
            if (listener != null) listener.accept(_currentBaudRate);
        }

        /** Builds the write-op closure for an {@link AsyncUsbWritePump#forD2xx} worker. The
         *  pump's single worker calls this; we re-enter {@code d2xxOp} for thread-safe error
         *  translation. The returned op must only be called from one thread (D2XX is single-URB);
         *  the pump enforces that with workerCount=1. */
        public AsyncUsbWritePump.WriteOp asyncWriteOp() {
            return (buf, len, timeoutMs) -> {
                ensureOpen();
                final int writeLen = Math.min(len, buf.length);
                return d2xxOp("writing D2XX data (async)", () -> {
                    final int n = _ftDevice.write(buf, writeLen, true, timeoutMs);
                    if (n < writeLen) {
                        throw new IOException("D2XX short write: " + n + "/" + writeLen);
                    }
                    return n;
                });
            };
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
            d2xxOp("setting D2XX control line DTR", () -> {
                final boolean ok = value ? _ftDevice.setDtr() : _ftDevice.clrDtr();
                if (!ok) {
                    throw new IOException("Failed to set DTR");
                }
                _dtr = value;
                return null;
            });
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
            d2xxOp("setting D2XX control line RTS", () -> {
                final boolean ok = value ? _ftDevice.setRts() : _ftDevice.clrRts();
                if (!ok) {
                    throw new IOException("Failed to set RTS");
                }
                _rts = value;
                return null;
            });
        }

        @Override
        public EnumSet<ControlLine> getControlLines() throws IOException {
            ensureOpen();
            // One control transfer reads all modem bits atomically, not four separate calls.
            final int status = d2xxOp("reading D2XX modem status", _ftDevice::getModemStatus);
            final EnumSet<ControlLine> lines = EnumSet.noneOf(ControlLine.class);
            if (_rts)                                  lines.add(ControlLine.RTS);
            if ((status & D2xxManager.FT_CTS) != 0)    lines.add(ControlLine.CTS);
            if (_dtr)                                  lines.add(ControlLine.DTR);
            if ((status & D2xxManager.FT_DSR) != 0)    lines.add(ControlLine.DSR);
            if ((status & D2xxManager.FT_DCD) != 0)    lines.add(ControlLine.CD);
            if ((status & D2xxManager.FT_RI)  != 0)    lines.add(ControlLine.RI);
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
            d2xxOp("setting D2XX flow control", () -> {
                final byte XON  = 0x11; // DC1 — standard ASCII XON
                final byte XOFF = 0x13; // DC3 — standard ASCII XOFF
                final short flowMode = toD2xxFlowControl(flowControl);
                final boolean ok = _ftDevice.setFlowControl(flowMode, XON, XOFF);
                if (!ok) {
                    throw new IOException("Failed to set flow control: " + flowControl);
                }
                _flowControl = flowControl;
                return null;
            });
        }

        @Override
        public FlowControl getFlowControl() {
            return isOpen() ? _flowControl : FlowControl.NONE;
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
            final byte purgeFlags = (byte) (
                    (purgeReadBuffers  ? D2xxManager.FT_PURGE_RX : 0) |
                    (purgeWriteBuffers ? D2xxManager.FT_PURGE_TX : 0));
            if (purgeFlags == 0) {
                return;
            }
            d2xxOp("purging D2XX buffers", () -> {
                if (!_ftDevice.purge(purgeFlags)) {
                    throw new IOException("Failed to purge FTDI buffers");
                }
                return null;
            });
        }

        @Override
        public void setBreak(final boolean value) throws IOException {
            ensureOpen();
            d2xxOp("setting D2XX break condition", () -> {
                final boolean ok = value ? _ftDevice.setBreakOn() : _ftDevice.setBreakOff();
                if (!ok) {
                    throw new IOException("Failed to set break condition");
                }
                return null;
            });
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

        private boolean readModemStatusBit(final int mask) throws IOException {
            return (d2xxOp("reading D2XX modem status", _ftDevice::getModemStatus) & mask) != 0;
        }

    }
}
