package android.hardware.usb;

import java.util.ArrayDeque;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.CopyOnWriteArrayList;

/** A USB connection for JVM unit tests. It lives in this package because UsbDeviceConnection has no public constructor. */
public final class FakeUsbDeviceConnection extends UsbDeviceConnection {
    /** The descriptor of a connection unless a test gives it another; no result code of the serial package. */
    public static final int FILE_DESCRIPTOR = 37;

    public boolean closed;
    public int fileDescriptor = FILE_DESCRIPTOR;
    /** The requests that a test's UsbRequest has queued with the device, oldest first. */
    public final Queue<UsbRequest> queuedRequests = new ArrayDeque<>();
    /** What a control transfer returns: the bytes it transferred, or a negative number if it failed. */
    public int controlTransferResult;
    /** The length of each bulk transfer, oldest first. A port's write thread makes them, not the test's. */
    public final List<Integer> bulkTransferLengths = new CopyOnWriteArrayList<>();

    @Override
    public void close() {
        closed = true;
    }

    @Override
    public int getFileDescriptor() {
        return fileDescriptor;
    }

    @Override
    public int controlTransfer(final int requestType, final int request, final int value, final int index,
                               final byte[] buffer, final int length, final int timeout) {
        return controlTransferResult;
    }

    /** Transfers all of the data at once, as to a device that takes it as fast as it is sent. */
    @Override
    public int bulkTransfer(final UsbEndpoint endpoint, final byte[] buffer, final int length, final int timeout) {
        bulkTransferLengths.add(length);
        return length;
    }

    /** Hands out the oldest queued request as completed, with no input, or null if none is queued. */
    @Override
    public UsbRequest requestWait() {
        return queuedRequests.poll();
    }
}
