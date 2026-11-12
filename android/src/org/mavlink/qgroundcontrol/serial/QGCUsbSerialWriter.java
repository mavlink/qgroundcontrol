package org.mavlink.qgroundcontrol.serial;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.io.IOException;
import java.util.Arrays;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * The output of one open port. It takes the chunks that C++ hands over and sends them to the device on a thread
 * of its own, in order, so that C++ never waits for a transfer.
 *
 * <p>The port serializes {@link #queue} and {@link #stop}. The writer's own lock guards only which thread is the
 * write thread: a thread whose transfer fails gives that up itself.
 */
final class QGCUsbSerialWriter {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialWriter";
    // C++ hands over one chunk and waits to hear that it was written, so more than a few waiting means that
    // something is wrong.
    static final int QUEUE_CHUNKS = 4;
    // A chunk goes to the driver in pieces of this size, and a discard stops it between two of them: a piece that
    // the driver has cannot be taken back. A multiple of every USB packet size, so that the device gets the same
    // transfers as from one write of the whole chunk.
    private static final int PIECE_BYTES = 1024;

    /** What the write thread tells the port. */
    interface Listener {
        /** The thread is done with a chunk of this length: it has sent it, or a discard kept it from sending it. */
        void chunkDone(int length);

        /** A transfer failed. The thread writes no more, and the chunks that waited behind the failed one are lost. */
        void writeFailed(Exception e);
    }

    /** A chunk that {@link #queue} accepted, with what {@code _generation} was at the time. */
    private record QueuedChunk(byte[] data, int generation) {}

    private final int _portId;
    private final UsbSerialPort _driverPort;
    private final Listener _listener;
    private final BlockingQueue<QueuedChunk> _queue = new ArrayBlockingQueue<>(QUEUE_CHUNKS);
    // Counts the discards. The write thread sends a chunk only for as long as this is what it was when the chunk
    // was queued, so a discard stops every chunk queued before it and none queued after it.
    private final AtomicInteger _generation = new AtomicInteger();
    private volatile boolean _stopped;
    private Thread _thread;

    QGCUsbSerialWriter(final int portId, final UsbSerialPort driverPort, final Listener listener) {
        _portId = portId;
        _driverPort = driverPort;
        _listener = listener;
    }

    /**
     * Hands a chunk to the write thread, which this starts if need be. Returns false if the thread still has
     * {@code QUEUE_CHUNKS} chunks to write.
     */
    synchronized boolean queue(final byte[] chunk) {
        if (!_queue.offer(new QueuedChunk(chunk, _generation.get()))) {
            return false;
        }
        if (_thread == null || !_thread.isAlive()) {
            _thread = new Thread(this::writeQueuedChunks, "QGCUsbSerialWrite-" + _portId);
            _thread.start();
        }
        return true;
    }

    /**
     * Keeps the write thread from sending what is left of the chunks queued so far. They are only marked: the
     * thread drops and reports them, which keeps the reports in order and off the caller's thread.
     */
    void discardQueued() {
        _generation.incrementAndGet();
    }

    /**
     * Ends the write thread for good, as the port closes, and drops the chunks that it has not sent. The thread
     * sends no further piece of the chunk that it is in.
     */
    synchronized void stop() {
        _stopped = true;
        if (_thread != null) {
            // Ends its wait for a chunk. A write in flight fails when the driver's port closes.
            _thread.interrupt();
            _thread = null;
        }
        _queue.clear();
    }

    /** The write thread: sends what {@link #queue} accepts until the writer is stopped or a transfer fails. */
    private void writeQueuedChunks() {
        try {
            while (!_stopped) {
                final QueuedChunk chunk = _queue.take();
                final int length = chunk.data().length;
                QGCUsbSerialTrace.transfer("TX", _portId, chunk.data(), length);
                final int sent = sendUntilDiscarded(chunk);
                if (sent < length) {
                    QGCUsbSerialTrace.event(_portId, "TX discarded after " + sent + " of " + length + " bytes");
                }
                // The whole chunk, sent or not. C++ expects one report of every byte that it handed over, in
                // order, and leaves out the bytes that it discarded.
                _listener.chunkDone(length);
            }
        } catch (final InterruptedException e) {
            QGCLogger.d(TAG, "Write thread of port " + _portId + " ended by close");
        } catch (final IOException | RuntimeException e) {
            // Closing the port clears the driver's connection without locking, under a write still in flight.
            if (_stopped) {
                QGCLogger.d(TAG, "Port " + _portId + " was closed during a write");
                return;
            }
            // The driver has no timeout to report here, so the transfer itself failed.
            QGCLogger.w(TAG, "Write failed on port " + _portId + ": " + e);
            synchronized (this) {
                // This thread writes no more. Until C++ hears of the failure it may hand over a chunk, which then
                // takes a thread of its own; left to this one, it would be neither written nor reported.
                _queue.clear();
                if (_thread == Thread.currentThread()) {
                    _thread = null;
                }
            }
            _listener.writeFailed(e);
        }
    }

    /**
     * Sends a chunk to the device a piece at a time, until all of it is sent, {@link #discardQueued} has
     * discarded it or the writer is stopped. Returns how many of its bytes were sent.
     */
    private int sendUntilDiscarded(final QueuedChunk chunk) throws IOException {
        final byte[] data = chunk.data();
        int sent = 0;
        while (sent < data.length && !_stopped && chunk.generation() == _generation.get()) {
            final int pieceLength = Math.min(data.length - sent, PIECE_BYTES);
            // The driver writes from the start of an array, so every piece but the first is copied.
            final byte[] piece = (sent == 0) ? data : Arrays.copyOfRange(data, sent, sent + pieceLength);
            // No timeout: a device that stops taking data holds the write until the port closes.
            _driverPort.write(piece, pieceLength, 0);
            sent += pieceLength;
        }
        return sent;
    }
}
