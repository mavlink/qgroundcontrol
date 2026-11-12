package org.mavlink.qgroundcontrol.serial;

import android.os.Process;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * The input of one open port. It reads the device on a thread of its own and hands what arrives to the port,
 * which passes it on to C++. While C++ has no room for more, the thread reads no further.
 *
 * <p>The port serializes {@link #start} and {@link #stop}, and starts no reader that it has stopped. The reader's
 * own lock guards which thread is the read thread, and that thread waits on it while C++ has no room.
 */
final class QGCUsbSerialReader {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialReader";
    // A read transfer needs room for a whole USB packet, 1024 bytes at most. Each read is passed to C++ whole, so
    // this must also stay within kMaxNativePayloadBytes in AndroidSerial.cc.
    private static final int READ_BUF_SIZE = 2048;
    // The read transfers that stay queued with the device, so that input which arrives while the read thread
    // passes some to C++ has somewhere to go.
    private static final int READ_QUEUE_BUFFERS = 2;
    // What the library's read throws when the device does not answer the status request that follows a read
    // transfer which came back empty. That transfer is queued again by then, so the port can be read on.
    static final String STATUS_CHECK_FAILED = "USB get_status request failed";
    // A device that stays attached may fail the status check now and then, as one behind a hub does. This many
    // failures in a row end the reading as any other failed read does.
    private static final int STATUS_CHECK_FAILURES_TO_STOP = 3;

    /** What the read thread tells the port. */
    interface Listener {
        /**
         * The first {@code length} bytes of {@code data} have arrived. The thread reuses the array for its next
         * read. Returns whether C++ has room for more input.
         */
        boolean inputArrived(byte[] data, int length);

        /** A read failed, as the one in flight does when the port closes. The thread reads no more. */
        void readFailed(Exception e);
    }

    private final int _portId;
    private final UsbSerialPort _driverPort;
    private final Listener _listener;
    private volatile boolean _stopped;
    // Set while C++ has stopped reading; see suspend().
    private volatile boolean _suspended;
    // Counts the calls of start(). The read thread waits for C++ to have room only for as long as this is what it
    // was before the thread handed over the input that C++ had no room behind: C++ may start reading again before
    // the thread has got as far as waiting.
    private final AtomicInteger _starts = new AtomicInteger();
    // Null until reading starts, and again once the thread has failed or the reader is stopped.
    private Thread _thread;
    private boolean _hasStarted;

    QGCUsbSerialReader(final int portId, final UsbSerialPort driverPort, final Listener listener) {
        _portId = portId;
        _driverPort = driverPort;
        _listener = listener;
    }

    /**
     * Starts handing the port's input to the listener, or resumes it after {@link #suspend} or after C++ had no
     * room for more. Returns {@code OK}, or {@code FAILED} if the driver takes no read queue.
     */
    @QGCUsbSerialResult.Code
    synchronized int start() {
        // Ends the read thread's wait for C++ to have room. Counted before a read thread is started, which would
        // otherwise take this start for one that follows its first input.
        _starts.incrementAndGet();
        notifyAll();
        if (_thread == null) {
            try {
                _driverPort.setReadQueue(READ_QUEUE_BUFFERS, READ_BUF_SIZE);
            } catch (final IllegalStateException | IllegalArgumentException e) {
                QGCLogger.e(TAG, "Cannot start reading port " + _portId, e);
                QGCUsbSerialLastError.set(e);
                return QGCUsbSerialResult.FAILED;
            }
            _hasStarted = true;
            _thread = new Thread(this::readInput, "QGCUsbSerialRead-" + _portId);
            _thread.start();
        }
        _suspended = false;
        return QGCUsbSerialResult.OK;
    }

    /**
     * Has the port drop its input until {@link #start}.
     *
     * <p>The read thread keeps running: it cannot be stopped while it waits for input, which would take until
     * input arrived or the port closed. If it waits for C++ to have room, it reads on, for the input to be
     * dropped.
     */
    synchronized void suspend() {
        _suspended = true;
        notifyAll();
    }

    /** Whether the port is to drop its input: C++ has stopped reading. */
    boolean isSuspended() {
        return _suspended;
    }

    /**
     * Whether reading has started and is not suspended. That is so also while the read thread waits for C++ to
     * have room for more input.
     */
    synchronized boolean isReading() {
        return _thread != null && !_suspended;
    }

    /** Whether reading has ever started: the driver's port has a read queue from then on. */
    synchronized boolean hasStarted() {
        return _hasStarted;
    }

    /**
     * Ends the read thread for good, as the port closes. The thread cannot be interrupted while it waits for
     * input: its read fails when the driver's port closes, which ends it. If it waits for C++ to have room
     * instead, this ends it.
     */
    synchronized void stop() {
        _stopped = true;
        _thread = null;
        notifyAll();
    }

    /** The read thread: hands the port's input on until the reader is stopped or a read fails. */
    private void readInput() {
        // The priority of the most urgent audio threads, so that the thread runs as soon as input arrives. While
        // it does not run, input has nowhere to go but the read transfers queued with the device,
        // READ_QUEUE_BUFFERS of READ_BUF_SIZE bytes, which a link at 921600 baud fills in under 50 ms. Whether a
        // lower priority would lose input has not been measured.
        try {
            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final SecurityException | IllegalArgumentException e) {
            // The platform may refuse the priority, and the thread reads without it. Uncaught, the refusal would
            // end the app.
            QGCLogger.w(TAG, "Read thread of port " + _portId + " keeps its default priority: " + e);
        }
        // The driver takes only a buffer of the size of its read queue's, and no timeout.
        final byte[] buffer = new byte[READ_BUF_SIZE];
        int statusCheckFailures = 0;
        try {
            while (!_stopped) {
                final int length;
                try {
                    length = _driverPort.read(buffer, 0);
                } catch (final IOException e) {
                    statusCheckFailures++;
                    if (!canReadOnAfter(e, statusCheckFailures)) {
                        throw e;
                    }
                    QGCLogger.w(TAG,
                            "Reading on after failed status check " + statusCheckFailures + " of port " + _portId);
                    continue;
                }
                statusCheckFailures = 0;
                if (length > 0) {
                    final int starts = _starts.get();
                    if (!_listener.inputArrived(buffer, length)) {
                        awaitRoom(starts);
                    }
                }
            }
        } catch (final IOException | RuntimeException | InterruptedException e) {
            synchronized (this) {
                // Before C++ hears of the failure, since it may start reading again at once.
                if (_thread == Thread.currentThread()) {
                    _thread = null;
                }
            }
            _listener.readFailed(e);
        }
    }

    /**
     * Whether the port can be read again after a read threw {@code e}: only after the library's status check
     * failed, for fewer than {@code STATUS_CHECK_FAILURES_TO_STOP} times in a row. A device that is unplugged
     * fails the check too, and uses the tries up at once. After any other failure the library may have no read
     * transfer queued, and a read would wait for one without end.
     */
    private boolean canReadOnAfter(final IOException e, final int statusCheckFailures) {
        return STATUS_CHECK_FAILED.equals(e.getMessage()) && statusCheckFailures < STATUS_CHECK_FAILURES_TO_STOP
                && !_stopped;
    }

    /**
     * Holds the read thread until C++ has room for more input: until it starts reading again, which it may have
     * done since {@code starts} was read, stops reading, or closes the port. The device still fills the read
     * transfers that are queued with it, {@code READ_QUEUE_BUFFERS} of {@code READ_BUF_SIZE} bytes; no more is
     * asked of it meanwhile, so the adapter then holds its input back as a serial port of the system would. The
     * wait is here and not in the call into C++, where nothing could end it.
     */
    private void awaitRoom(final int starts) throws InterruptedException {
        synchronized (this) {
            if (!isRoomAwaited(starts)) {
                return;
            }
            QGCUsbSerialTrace.event(_portId, "RX paused");
            while (isRoomAwaited(starts)) {
                wait();
            }
        }
        if (!_stopped) {
            QGCUsbSerialTrace.event(_portId, "RX resumed");
        }
    }

    private boolean isRoomAwaited(final int starts) {
        return !_stopped && !_suspended && _starts.get() == starts;
    }
}
