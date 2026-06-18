package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

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

/**
 * D2XX-backed {@link UsbSerialPort}, owned by {@link QGCFtdiSerialDriver}; uses {@link D2xxLibrary} for the D2XX runtime and param translation.
 *
 * <p>Writes use {@code FT_Device.write(wait=true)}, paced at wire rate by USB 2.0 bulk flow control (the FT232R NAKs its OUT endpoint when full),
 * so no Java-side rate limiter is needed; {@link #cancelPendingWrites} preempts a write parked on a wedged wire so close() can't stall.</p>
 */
final class QGCFtdiSerialPort implements UsbSerialPort {

    private static final String TAG = QGCFtdiSerialPort.class.getSimpleName();

    private final QGCFtdiSerialDriver _driver;
    private final UsbDevice _device;
    private final int _portNumber;

    private volatile FT_Device _ftDevice;
    private FlowControl _flowControl = FlowControl.NONE;
    private boolean _dtr;
    private boolean _rts;
    private int _readQueueBufferCount;
    private int _readQueueBufferSize;
    private short _lastLineStatus;

    /** Sticky once close() starts, so a chunked write loop bails at the next chunk boundary. */
    private volatile boolean _writesCancelled;
    /** Thread parked inside the blocking D2XX write, or null; interrupting it makes FT_Device.write's Future.get throw, cancelling the UsbRequest. */
    private volatile Thread _blockedWriter;
    /** Guards _writesCancelled and the _blockedWriter publish so cancelPendingWrites() and write() can't race the cancelled-check + thread-publish pair. */
    private final Object _writeGate = new Object();

    /** Preempts an in-flight {@link #write} so close() can't sit behind a wedged-wire D2XX write holding lifecycleLock; called from close() before it takes the lock. Sticky — close is terminal. */
    public void cancelPendingWrites() {
        synchronized (_writeGate) {
            _writesCancelled = true;
            if (_blockedWriter != null) {
                _blockedWriter.interrupt();
            }
        }
    }

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

    private int d2xxLocationForPort() {
        int interfaceId = _portNumber;
        final int interfaceCount = _device.getInterfaceCount();
        if (_portNumber >= 0 && _portNumber < interfaceCount) {
            interfaceId = _device.getInterface(_portNumber).getId();
        }
        return D2xxLibrary.d2xxLocation(_device.getDeviceId(), interfaceId);
    }

    public int readTimeoutForIoManager() {
        return D2xxLibrary.D2XX_READ_TIMEOUT_MS;
    }

    // D2XX does I/O internally; bulk endpoints are exposed only so the I/O manager can size its read buffer via getMaxPacketSize(). Resolved lazily, never fails open().
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
        // D2XX manages its own read buffering; values are recorded for diagnostic logging in open().
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
    private interface D2xxOp<T> { T run(FT_Device device) throws IOException; }

    /** Snapshots {@code _ftDevice} so a concurrent close() nulling the field can't NPE a caller mid-op; ops on the snapshot fail with IOException via d2xxOp once closed. */
    private <T> T d2xxOp(final String desc, final D2xxOp<T> op) throws IOException {
        final FT_Device device = _ftDevice;
        if (device == null || !device.isOpen()) {
            throw new IOException("Port not open");
        }
        try { return op.run(device); }
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
        // D2XX path passes null — the caller closes its probe connection before D2XX's openDevice() to avoid the double-open race; QGCFtdiSerialPort never uses connection for I/O.
        // Snapshot the (manager, context) pair atomically so a concurrent cleanup() can't null one between this check and the D2XX open below.
        final D2xxLibrary.Handle handle = D2xxLibrary.handle();
        if (handle == null || !D2xxLibrary.isFtdiDevice(_device)) {
            throw new IOException("D2XX manager unavailable or device is not an FTDI device");
        }

        QGCLogger.d(TAG, "D2XX opening: dev=" + _device.getDeviceName()
                + " vid=0x" + Integer.toHexString(_device.getVendorId())
                + " pid=0x" + Integer.toHexString(_device.getProductId())
                + " serial=" + _device.getSerialNumber()
                + " ifaces=" + _device.getInterfaceCount()
                + " readQueue=" + _readQueueBufferCount + "x" + _readQueueBufferSize);

        // Refresh D2XX's device list immediately before openByLocation (TN_147 pattern); otherwise findDevice() may iterate a stale list from the prober's check and return null for an attached device.
        try {
            final int count = handle.manager.createDeviceInfoList(handle.appContext);
            QGCLogger.d(TAG, "D2XX createDeviceInfoList pre-open: count=" + count);
        } catch (Throwable t) {
            QGCLogger.w(TAG, "createDeviceInfoList pre-open failed (" + t.getClass().getName() + "): " + t.getMessage());
        }

        final int d2xxLocation = d2xxLocationForPort();
        FT_Device d2xxDevice = null;
        Throwable openThrowable = null;
        try {
            d2xxDevice = handle.manager.openByLocation(
                    handle.appContext, d2xxLocation, D2xxLibrary.makeDriverParameters());
        } catch (Throwable t) {
            openThrowable = t;
        }

        if (d2xxDevice == null || !d2xxDevice.isOpen()) {
            // openByLocation binds the requested Android USB interface instead of always interface 0 on multi-port FT2232/FT4232; FTDI logs kernel/permission failures to tag "FTDI_Device::".
            final String mode;
            if (openThrowable != null) {
                mode = "threw " + openThrowable.getClass().getName() + ": " + openThrowable.getMessage();
            } else if (d2xxDevice == null) {
                mode = "openByLocation returned null for location 0x"
                        + Integer.toHexString(d2xxLocation);
            } else {
                mode = "openByLocation returned an FT_Device but isOpen()=false for location 0x"
                        + Integer.toHexString(d2xxLocation);
            }
            QGCLogger.e(TAG, "D2XX open FAILED for " + _device.getDeviceName() + ": " + mode, openThrowable);
            throw new IOException("Failed to open D2XX FTDI device " + _device.getDeviceName() + ": " + mode, openThrowable);
        }

        // connection unused on D2XX path; D2XX opens its own connection in openByLocation
        _ftDevice = d2xxDevice;
        QGCLogger.d(TAG, "D2XX open OK: " + _device.getDeviceName());

        // Reset bit mode to UART before configuring, defending against an FT232H/FT2232H left in MPSSE/bitbang by a prior app (else baud/data would apply to a chip still in bitbang).
        try {
            _ftDevice.setBitMode((byte) 0, D2xxManager.FT_BITMODE_RESET);
        } catch (Throwable t) {
            QGCLogger.w(TAG, "setBitMode RESET failed (" + t.getClass().getName() + "): " + t.getMessage());
        }

        // FTDI's default 16ms latency timer caps short-read flush at ~62 Hz; D2XX documents 2ms as the programmable minimum. Best-effort.
        try {
            _ftDevice.setLatencyTimer(D2xxLibrary.D2XX_LATENCY_TIMER_MS);
        } catch (Throwable t) {
            QGCLogger.w(TAG, "setLatencyTimer failed (" + t.getClass().getName() + "): " + t.getMessage());
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

    }

    @Override
    public int read(final byte[] dest, final int timeout) throws IOException {
        return read(dest, dest.length, timeout);
    }

    @Override
    public int read(final byte[] dest, final int length, final int timeout) throws IOException {
        final FT_Device device = _ftDevice;
        if (device == null || !device.isOpen()) {
            throw new IOException("Port not open");
        }
        if (dest == null) {
            throw new IOException("Null read buffer");
        }
        if (length <= 0) {
            return 0;
        }

        final int readLength = Math.min(length, dest.length);
        try {
            final int bytesRead = device.read(dest, readLength, timeout);
            return D2xxLibrary.normalizeReadResult(bytesRead, readLength);
        } catch (final IOException e) {
            throw e;
        } catch (Throwable t) {
            // Hot-unplug surfaces from D2XX as RuntimeException/NPE out of JNI (no declared IOException). Returning 0 would spin SerialInputOutputManager forever without onRunError,
            // so C++ never sees EXC_RESOURCE and the port stays "open"; throw so the IO manager exits and the listener-mute / token-stale path runs.
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
        if (src == null || length <= 0) {
            return;
        }
        final int writeLen = Math.min(length, src.length);
        // Publish _blockedWriter and re-check _writesCancelled inside _writeGate to close the lost-wakeup gap: a concurrent cancelPendingWrites() either sees our thread and interrupts, or this re-check catches its earlier flip.
        synchronized (_writeGate) {
            if (_writesCancelled) {
                throw new IOException("D2XX write cancelled (port closing)");
            }
            _blockedWriter = Thread.currentThread();
        }
        try {
            d2xxOp("writing D2XX data", (device) -> {
                // wait=true blocks until the bulk-OUT URB completes; the FT232R NAKs its OUT endpoint when full, so completion is wire-rate paced.
                final int n = device.write(src, writeLen, true, timeout);
                if (n < writeLen) {
                    throw new IOException("D2XX short write: " + n + "/" + writeLen);
                }
                return n;
            });
        } finally {
            synchronized (_writeGate) {
                _blockedWriter = null;
            }
            // FT_Device.write consumes any InterruptedException; clear a stray interrupt cancelPendingWrites may have set after the write so it can't leak into the C++ owner thread's next JNI call.
            Thread.interrupted();
        }
    }

    @Override
    public void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) throws IOException {
        d2xxOp("setting D2XX parameters", (device) -> {
            final boolean baudOk = device.setBaudRate(baudRate);
            final boolean charsOk = device.setDataCharacteristics(
                D2xxLibrary.toD2xxDataBits(dataBits),
                D2xxLibrary.toD2xxStopBits(stopBits),
                D2xxLibrary.toD2xxParity(parity));
            if (!baudOk || !charsOk) {
                throw new IOException("Failed to set FTDI serial parameters");
            }
            // Canonical FTDI pattern: purge both FIFOs after baud/data is set so stale bytes captured at the previous (wrong) baud don't reach the parser.
            device.purge((byte) (D2xxManager.FT_PURGE_RX | D2xxManager.FT_PURGE_TX));
            return null;
        });
    }

    @Override
    public boolean getCD() throws IOException {
        return readModemStatusBit(D2xxManager.FT_DCD);
    }

    @Override
    public boolean getCTS() throws IOException {
        return readModemStatusBit(D2xxManager.FT_CTS);
    }

    @Override
    public boolean getDSR() throws IOException {
        return readModemStatusBit(D2xxManager.FT_DSR);
    }

    @Override
    public boolean getDTR() throws IOException {
        ensureOpen();
        return _dtr;
    }

    @Override
    public void setDTR(final boolean value) throws IOException {
        d2xxOp("setting D2XX control line DTR", (device) -> {
            final boolean ok = value ? device.setDtr() : device.clrDtr();
            if (!ok) {
                throw new IOException("Failed to set DTR");
            }
            _dtr = value;
            return null;
        });
    }

    @Override
    public boolean getRI() throws IOException {
        return readModemStatusBit(D2xxManager.FT_RI);
    }

    @Override
    public boolean getRTS() throws IOException {
        ensureOpen();
        return _rts;
    }

    @Override
    public void setRTS(final boolean value) throws IOException {
        d2xxOp("setting D2XX control line RTS", (device) -> {
            final boolean ok = value ? device.setRts() : device.clrRts();
            if (!ok) {
                throw new IOException("Failed to set RTS");
            }
            _rts = value;
            return null;
        });
    }

    @Override
    public EnumSet<ControlLine> getControlLines() throws IOException {
        // One control transfer reads all modem bits atomically, not four separate calls.
        final int status = d2xxOp("reading D2XX modem status", (device) -> device.getModemStatus());
        // Best-effort UART line-status read (overrun/parity/framing/break), logged on change so persistent cabling/baud errors surface in logcat without spamming.
        final FT_Device device = _ftDevice;
        try {
            final short lineStatus = device != null ? device.getLineStatus() : 0;
            if (lineStatus != _lastLineStatus) {
                if (lineStatus != 0) {
                    QGCLogger.w(TAG, "D2XX line status non-zero on " + _device.getDeviceName()
                            + ":" + describeLineStatus(lineStatus));
                } else {
                    QGCLogger.i(TAG, "D2XX line status cleared on " + _device.getDeviceName());
                }
                _lastLineStatus = lineStatus;
            }
        } catch (Throwable t) {
            // getLineStatus is best-effort diagnostic; don't fail getControlLines on its failure.
        }
        final EnumSet<ControlLine> lines = EnumSet.noneOf(ControlLine.class);
        if (_rts)                                  lines.add(ControlLine.RTS);
        if ((status & D2xxManager.FT_CTS) != 0)    lines.add(ControlLine.CTS);
        if (_dtr)                                  lines.add(ControlLine.DTR);
        if ((status & D2xxManager.FT_DSR) != 0)    lines.add(ControlLine.DSR);
        if ((status & D2xxManager.FT_DCD) != 0)    lines.add(ControlLine.CD);
        if ((status & D2xxManager.FT_RI)  != 0)    lines.add(ControlLine.RI);
        return lines;
    }

    private static String describeLineStatus(final short lineStatus) {
        final StringBuilder sb = new StringBuilder();
        if ((lineStatus & D2xxManager.FT_OE) != 0) sb.append(" overrun");
        if ((lineStatus & D2xxManager.FT_PE) != 0) sb.append(" parity");
        if ((lineStatus & D2xxManager.FT_FE) != 0) sb.append(" framing");
        if ((lineStatus & D2xxManager.FT_BI) != 0) sb.append(" break");
        return sb.toString();
    }

    @Override
    public EnumSet<ControlLine> getSupportedControlLines() {
        return EnumSet.of(ControlLine.RTS, ControlLine.CTS, ControlLine.DTR, ControlLine.DSR, ControlLine.CD, ControlLine.RI);
    }

    @Override
    public void setFlowControl(final FlowControl flowControl) throws IOException {
        if (flowControl == FlowControl.XON_XOFF_INLINE) {
            throw new IOException("XON/XOFF inline flow control is not supported by D2XX adapter");
        }
        d2xxOp("setting D2XX flow control", (device) -> {
            final byte XON  = 0x11; // DC1 — standard ASCII XON
            final byte XOFF = 0x13; // DC3 — standard ASCII XOFF
            final short flowMode = D2xxLibrary.toD2xxFlowControl(flowControl);
            final boolean ok = device.setFlowControl(flowMode, XON, XOFF);
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
        final byte purgeFlags = (byte) (
                (purgeReadBuffers  ? D2xxManager.FT_PURGE_RX : 0) |
                (purgeWriteBuffers ? D2xxManager.FT_PURGE_TX : 0));
        if (purgeFlags == 0) {
            return;
        }
        d2xxOp("purging D2XX buffers", (device) -> {
            if (!device.purge(purgeFlags)) {
                throw new IOException("Failed to purge FTDI buffers");
            }
            return null;
        });
    }

    @Override
    public void setBreak(final boolean value) throws IOException {
        d2xxOp("setting D2XX break condition", (device) -> {
            final boolean ok = value ? device.setBreakOn() : device.setBreakOff();
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
        return (d2xxOp("reading D2XX modem status", (device) -> device.getModemStatus()) & mask) != 0;
    }

    /**
     * D2XX-backed {@link UsbSerialDriver}; the port impl is the enclosing {@link QGCFtdiSerialPort} and the D2XX runtime + capability checks live in {@link D2xxLibrary}.
     * This class is just the {@code UsbSerialDriver} hookup.
     */
    public static final class QGCFtdiSerialDriver implements UsbSerialDriver {

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

        @Override
        public UsbDevice getDevice() {
            return _device;
        }

        @Override
        public List<UsbSerialPort> getPorts() {
            return _ports;
        }
    }
}
