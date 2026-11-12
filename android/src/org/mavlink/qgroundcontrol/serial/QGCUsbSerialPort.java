package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.io.IOException;
import java.util.Arrays;
import java.util.EnumSet;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * One open USB serial port: the driver's port, its USB connection, its {@link QGCUsbSerialReader}, which has
 * the read thread, and its {@link QGCUsbSerialWriter}, which has the write thread.
 *
 * <p>{@code _lock} serializes opening, closing and the starting of the I/O threads. Transfers run without
 * it, so closing a port never waits behind a blocked transfer; the driver fails that transfer instead. A thread
 * whose transfer failed waits on it to learn whether the port is being closed.
 *
 * <p>The driver's port is this port's alone ({@link QGCUsbSerialDevices#open}). An I/O thread that has not ended
 * by the time the port is closed therefore finds its driver port closed, whatever is opened on the device next.
 */
final class QGCUsbSerialPort {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialPort";
    // An FTDI chip holds back a partly filled packet for this long, 16 ms by default. Each expiry on an idle
    // port wakes the read thread, so a much lower value costs CPU: 2 ms took 8% of a tablet core, against 3%.
    private static final int FTDI_LATENCY_TIMER_MS = 8;
    // An unplug fails a transfer before Android reports that the device detached. A failed transfer therefore
    // waits this long, at most, for that report, on which the port is closed. A high-speed hub tells of an unplug
    // only when it is next asked, up to 256 ms later, so the wait is longer than that.
    private static final long DETACH_WAIT_MS = 500;

    /** A small number that names the port in logs and in the trace. */
    final int id;
    /** Identifies the port to both sides: C++ chooses it and passes it in every call, and gets it in callbacks. */
    final long handle;
    final QGCUsbSerialDevices.PortAddress address;

    private final UsbSerialPort _driverPort;
    private final QGCUsbSerialCallbacks _callbacks;
    private final Object _lock = new Object();
    private volatile long _detachWaitMs = DETACH_WAIT_MS;
    // The handle, which is never 0, for as long as C++ should hear from this port. Cleared as close() begins, so
    // that late callbacks from the I/O threads are dropped; the port counts as closed from then on.
    private volatile long _token;
    // Whether C++ has been told since it last started reading that the port can no longer transfer data. It is
    // told once, so the port takes no output meanwhile: C++ would not hear of its loss.
    private final AtomicBoolean _resourceErrorReported = new AtomicBoolean();
    private final QGCUsbSerialReader _reader;
    private final QGCUsbSerialWriter _writer;
    private int _fileDescriptor = -1;

    QGCUsbSerialPort(final int id, final long handle, final QGCUsbSerialDevices.PortAddress address,
                     final UsbSerialPort driverPort, final QGCUsbSerialCallbacks callbacks) {
        this.id = id;
        this.handle = handle;
        this.address = address;
        _driverPort = driverPort;
        _token = handle;
        _callbacks = callbacks;
        _reader = new QGCUsbSerialReader(id, driverPort, new QGCUsbSerialReader.Listener() {
            @Override
            public boolean inputArrived(final byte[] data, final int length) {
                return QGCUsbSerialPort.this.inputArrived(data, length);
            }

            @Override
            public void readFailed(final Exception e) {
                QGCUsbSerialPort.this.readFailed(e);
            }
        });
        _writer = new QGCUsbSerialWriter(id, driverPort, new QGCUsbSerialWriter.Listener() {
            @Override
            public void chunkDone(final int length) {
                final long token = _token;
                if (token != 0) {
                    _callbacks.bytesWritten(token, length);
                }
            }

            @Override
            public void writeFailed(final Exception e) {
                reportTransferFailure(QGCUsbSerialResourceError.WRITE_FAILED, e);
            }
        });
    }

    boolean isOpen() {
        return !isClosed() && _driverPort.isOpen();
    }

    /** Whether {@link #close} has begun. It may still wait for {@code _lock}, with the driver's port open. */
    private boolean isClosed() {
        return _token == 0;
    }

    /** Returns the USB connection's file descriptor, or -1. */
    int fileDescriptor() {
        synchronized (_lock) {
            return _fileDescriptor;
        }
    }

    /**
     * Opens the USB connection and the driver's port, which blocks on USB transfers. Returns the
     * {@link QGCUsbSerialResult} that says how it went. The caller has made sure that QGC may open the device;
     * for one that it may not, Android opens no connection.
     */
    @QGCUsbSerialResult.Code
    int open(final QGCUsbSerialHost host) {
        final UsbDevice device = _driverPort.getDevice();
        synchronized (_lock) {
            if (isClosed()) {
                // Its device detached while the open was waiting.
                return QGCUsbSerialResult.DEVICE_NOT_FOUND;
            }
            final UsbDeviceConnection connection = host.openDevice(device);
            if (connection == null) {
                QGCLogger.e(TAG, "No USB connection to " + device.getDeviceName());
                QGCUsbSerialLastError.set("Android opened no USB connection");
                return QGCUsbSerialResult.FAILED;
            }
            try {
                _driverPort.open(connection);
            } catch (final IOException | RuntimeException e) {
                connection.close();
                QGCLogger.e(TAG, "Error opening the driver for " + device.getDeviceName(), e);
                QGCUsbSerialLastError.set(e);
                return QGCUsbSerialResult.FAILED;
            }
            _fileDescriptor = connection.getFileDescriptor();
            if (_driverPort instanceof FtdiSerialDriver.FtdiSerialPort ftdiPort) {
                try {
                    ftdiPort.setLatencyTimer(FTDI_LATENCY_TIMER_MS);
                } catch (final IOException e) {
                    QGCLogger.w(TAG, "Could not set the FTDI latency timer: " + e.getMessage());
                }
            }
            return QGCUsbSerialResult.OK;
        }
    }

    /** Stops the I/O threads and closes the driver's port. Returns false if the driver failed to close. */
    boolean close() {
        _token = 0;
        synchronized (_lock) {
            // Ends the wait of a thread whose transfer failed.
            _lock.notifyAll();
            // A read or a write in flight fails when the driver's port closes below.
            _reader.stop();
            _writer.stop();
            _fileDescriptor = -1;
            if (!_driverPort.isOpen()) {
                return true;
            }
            try {
                _driverPort.close();
                return true;
            } catch (final IOException e) {
                QGCLogger.e(TAG, "Error closing port " + id, e);
                QGCUsbSerialLastError.set(e);
                return false;
            }
        }
    }

    /**
     * Closes the port because its device is gone, and tells the C++ port that still owned it. Whether the read
     * thread or Android's detach broadcast notices first, C++ gets one resource error.
     */
    void closeDisconnected() {
        final long token = _token;
        close();
        reportResourceError(token, QGCUsbSerialResourceError.DEVICE_DETACHED, "");
    }

    /**
     * Tells C++ that the port can no longer transfer data, unless it was told already since it last started
     * reading. {@code token} is what the caller read from {@code _token}: with 0 the port was closed by then, and
     * C++ is not told.
     */
    private void reportResourceError(final long token, @QGCUsbSerialResourceError.Reason final int reason,
                                     final String detail) {
        if (token != 0 && _resourceErrorReported.compareAndSet(false, true)) {
            _callbacks.resourceError(token, reason, detail);
        }
    }

    /**
     * Tells C++ that a transfer failed, unless the port is closed by then or C++ was told already. An unplug
     * fails a transfer before Android reports it, so this first waits a moment for that report: on it the port is
     * closed as disconnected ({@link #closeDisconnected}), which tells C++ of the unplug instead. Whoever else
     * closes the port during the wait tells C++ what there is to tell.
     */
    private void reportTransferFailure(@QGCUsbSerialResourceError.Reason final int reason, final Exception e) {
        awaitClose();
        reportResourceError(_token, reason, detailOf(e));
    }

    /** Waits until the port is closed, for {@code _detachWaitMs} at most. */
    private void awaitClose() {
        final long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(_detachWaitMs);
        synchronized (_lock) {
            try {
                while (!isClosed()) {
                    final long remainingNanos = deadline - System.nanoTime();
                    if (remainingNanos <= 0) {
                        return;
                    }
                    // Not in whole milliseconds, which would end the wait up to one early.
                    TimeUnit.NANOSECONDS.timedWait(_lock, remainingNanos);
                }
            } catch (final InterruptedException e) {
                // Only close() interrupts an I/O thread, so the port is closed.
                Thread.currentThread().interrupt();
            }
        }
    }

    /** Sets how long a failed transfer waits for its device to detach, which a test cannot sit out every time. */
    void setDetachWaitForTesting(final long milliseconds) {
        _detachWaitMs = milliseconds;
    }

    /** What a failure says of itself: the message of its exception or, if that has none, the exception's name. */
    private static String detailOf(final Exception e) {
        final String message = e.getMessage();
        return (message != null) ? message : e.getClass().getSimpleName();
    }

    // ------------------------------------------------------------------------
    // Reading
    // ------------------------------------------------------------------------

    /**
     * Starts delivering the port's input to C++, or resumes it after {@link #stopReading} or after C++ had no
     * room for more. Returns the {@link QGCUsbSerialResult} that says how it went: {@code NOT_OPEN} if the port
     * is closed, as it is once its device has detached, which C++ takes for the loss of the device.
     */
    @QGCUsbSerialResult.Code
    int startReading() {
        synchronized (_lock) {
            if (!isOpen()) {
                QGCLogger.w(TAG, "Cannot start reading: port " + id + " is not open");
                return QGCUsbSerialResult.NOT_OPEN;
            }
            @QGCUsbSerialResult.Code final int result = _reader.start();
            if (result == QGCUsbSerialResult.OK) {
                // C++ asks this again once it has been told of an error, also of one that left the read thread
                // running, so it has to be told of the next.
                _resourceErrorReported.set(false);
            }
            return result;
        }
    }

    /** Stops delivering the port's input to C++ until {@link #startReading}; what arrives meanwhile is dropped. */
    void stopReading() {
        _reader.suspend();
    }

    /**
     * Whether C++ has started reading and not stopped it. That is so also while the read thread waits for C++ to
     * have room for more input.
     */
    boolean isReading() {
        return _reader.isReading();
    }

    /**
     * Passes the first {@code length} bytes of {@code data} on to C++, unless that has stopped reading or closed
     * the port. Returns whether C++ has room for more input, which it always has for input that is dropped.
     */
    boolean inputArrived(final byte[] data, final int length) {
        final long token = _token;
        if (token == 0 || _reader.isSuspended()) {
            return true;
        }
        QGCUsbSerialTrace.transfer("RX", id, data, length);
        // Not copied: C++ copies the bytes before this returns, so the read thread can reuse the array.
        return _callbacks.newData(token, data, length);
    }

    /**
     * Tells C++ that the port can no longer transfer data, unless it closed the port or was told already. Waits
     * first, for a moment, to learn whether the device was unplugged.
     */
    void readFailed(final Exception e) {
        if (isClosed()) {
            QGCLogger.d(TAG, "I/O stopped after close of port " + id + ": " + e);
            return;
        }
        QGCLogger.w(TAG, "I/O stopped for port " + id + ": " + e);
        reportTransferFailure(QGCUsbSerialResourceError.READ_FAILED, e);
    }

    // ------------------------------------------------------------------------
    // Read / write
    // ------------------------------------------------------------------------

    /**
     * Reads from the port directly and returns the bytes read, possibly none. It reads nothing once reading has
     * been started: the read thread then delivers the input to C++, and two readers would each get part of it.
     */
    byte[] read(final int length, final int timeoutMs) throws IOException {
        // Not only while the read thread runs: the driver rejects a read with a timeout once its port has a read
        // queue.
        if (_reader.hasStarted()) {
            QGCLogger.w(TAG, "Cannot read port " + id + " directly: its input goes to its read thread");
            return new byte[0];
        }
        final byte[] buffer = new byte[length];
        final int bytesRead = _driverPort.read(buffer, timeoutMs);
        return (bytesRead < length) ? Arrays.copyOf(buffer, Math.max(bytesRead, 0)) : buffer;
    }

    /**
     * Returns the bytes written, fewer than {@code length} on timeout, or
     * {@link QGCUsbSerialResult#IO_ERROR} if the port can no longer transfer data. 0 waits without limit.
     */
    int writeWithProgress(final byte[] data, final int length, final int timeoutMs) {
        QGCUsbSerialTrace.transfer("TX", id, data, length);
        try {
            _driverPort.write(data, length, timeoutMs);
            return length;
        } catch (final SerialTimeoutException e) {
            final int transferred = Math.max(0, Math.min(e.bytesTransferred, length));
            QGCLogger.d(TAG, "Write timed out after " + transferred + " of " + length + " bytes");
            QGCUsbSerialLastError.set(e);
            return transferred;
        } catch (final IOException e) {
            // The driver reports a timeout separately, so the transfer itself failed: the device is gone.
            QGCLogger.w(TAG, "Write failed on port " + id + ": " + e.getMessage());
            QGCUsbSerialLastError.set(e);
            return QGCUsbSerialResult.IO_ERROR;
        } catch (final RuntimeException e) {
            // Closing the port clears the driver's connection without locking, under a write still in flight.
            if (isOpen()) {
                throw e;
            }
            QGCLogger.d(TAG, "Port " + id + " was closed during a write");
            // The exception only tells how the driver stumbled over the cleared connection.
            QGCUsbSerialLastError.set("The port was closed during the write");
            return QGCUsbSerialResult.IO_ERROR;
        }
    }

    /**
     * Hands {@code length} bytes to the port's write thread. Returns {@code length} at once, or a
     * {@link QGCUsbSerialResult}: {@code NOT_OPEN} if the port is closed, {@code FAILED} if the thread still has
     * too much to write, {@code IO_ERROR} if C++ has been told since it last started reading that the port can no
     * longer transfer data. C++ hears of all {@code length} bytes at once: when the device has taken them, or
     * when {@link #purgeBuffers} has discarded those not yet sent. If the thread fails to write them, C++ is told
     * that the port can no longer transfer data.
     */
    int writeAsync(final byte[] data, final int length) {
        final byte[] chunk = (length == data.length) ? data : Arrays.copyOf(data, length);
        synchronized (_lock) {
            if (!isOpen()) {
                return QGCUsbSerialResult.NOT_OPEN;
            }
            if (_resourceErrorReported.get()) {
                QGCLogger.d(TAG, "Port " + id + " takes no output: it can no longer transfer data");
                QGCUsbSerialLastError.set("The port has reported that it can no longer transfer data");
                return QGCUsbSerialResult.IO_ERROR;
            }
            if (!_writer.queue(chunk)) {
                QGCLogger.w(TAG, "Write thread of port " + id + " still has " + QGCUsbSerialWriter.QUEUE_CHUNKS
                        + " chunks to write");
                QGCUsbSerialLastError.set(
                        "The write thread still has " + QGCUsbSerialWriter.QUEUE_CHUNKS + " chunks to write");
                return QGCUsbSerialResult.FAILED;
            }
        }
        return length;
    }

    // ------------------------------------------------------------------------
    // Port settings
    // ------------------------------------------------------------------------

    void setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity)
            throws IOException {
        _driverPort.setParameters(baudRate, dataBits, stopBits, parity);
        QGCUsbSerialTrace.event(id, "parameters: " + baudRate + " baud, " + dataBits + " data bits, stop bits "
                + stopBits + ", parity " + parity);
    }

    UsbSerialPort.FlowControl flowControl() {
        return _driverPort.getFlowControl();
    }

    EnumSet<UsbSerialPort.FlowControl> supportedFlowControl() {
        return _driverPort.getSupportedFlowControl();
    }

    void setFlowControl(final UsbSerialPort.FlowControl flowControl) throws IOException {
        if (_driverPort.getFlowControl() == flowControl) {
            return;
        }
        if (!_driverPort.getSupportedFlowControl().contains(flowControl)) {
            throw new UnsupportedOperationException("flow control " + flowControl + " is not supported");
        }
        _driverPort.setFlowControl(flowControl);
    }

    /** Under XON/XOFF flow control, returns false while the device has paused QGC's transmission. */
    boolean isXon() throws IOException {
        try {
            return _driverPort.getXON();
        } catch (final UnsupportedOperationException e) {
            // A driver without XON/XOFF flow control never pauses.
            return true;
        }
    }

    /**
     * Sets how long an FTDI chip holds back a partly filled packet, from 1 to 255 ms, replacing the value set
     * when the port was opened. No other chip has this timer.
     */
    void setLatencyTimer(final int milliseconds) throws IOException {
        if (milliseconds < 1 || milliseconds > 255) {
            throw new IllegalArgumentException("invalid latency timer of " + milliseconds + " ms");
        }
        if (!(_driverPort instanceof FtdiSerialDriver.FtdiSerialPort ftdiPort)) {
            throw new UnsupportedOperationException("port " + id + " has no latency timer");
        }
        ftdiPort.setLatencyTimer(milliseconds);
    }

    void setBreak(final boolean on) throws IOException {
        _driverPort.setBreak(on);
    }

    /**
     * Discards what the device has buffered in the given directions. A discard of output also keeps the write
     * thread from sending what is left of the chunks that {@link #writeAsync} has accepted, also where the driver
     * has no purge command; the thread reports those chunks all the same.
     */
    void purgeBuffers(final boolean input, final boolean output) throws IOException {
        if (output) {
            // Before the driver's purge, which not every driver has. The write thread reports the discarded
            // chunks: this thread is the C++ port's, in the middle of that port's own call.
            _writer.discardQueued();
        }
        try {
            _driverPort.purgeHwBuffers(output, input);
        } catch (final UnsupportedOperationException e) {
            // CDC-ACM and some other drivers have no purge command, so there is nothing more to discard.
        }
    }

    // ------------------------------------------------------------------------
    // Control lines
    // ------------------------------------------------------------------------

    /** The lines that are active. */
    EnumSet<UsbSerialPort.ControlLine> controlLines() throws IOException {
        return _driverPort.getControlLines();
    }

    /** The lines the port has at all. */
    EnumSet<UsbSerialPort.ControlLine> supportedControlLines() throws IOException {
        return _driverPort.getSupportedControlLines();
    }

    /** Sets DTR or RTS, the two lines the host drives. */
    void setOutputLine(final UsbSerialPort.ControlLine controlLine, final boolean on) throws IOException {
        if (!_driverPort.getSupportedControlLines().contains(controlLine)) {
            throw new UnsupportedOperationException(controlLine + " is not supported");
        }
        switch (controlLine) {
            case DTR -> _driverPort.setDTR(on);
            case RTS -> _driverPort.setRTS(on);
            default -> throw new UnsupportedOperationException(controlLine + " is not the host's to set");
        }
    }
}
