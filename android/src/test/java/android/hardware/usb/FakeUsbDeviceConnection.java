package android.hardware.usb;

import java.util.ArrayDeque;
import java.util.Queue;

/** A USB connection for JVM unit tests. It lives in this package because UsbDeviceConnection has no public constructor. */
public final class FakeUsbDeviceConnection extends UsbDeviceConnection {
    public boolean closed;
    /** The requests that a test's UsbRequest has queued with the device, oldest first. */
    public final Queue<UsbRequest> queuedRequests = new ArrayDeque<>();
    /** What a control transfer returns: the bytes it transferred, or a negative number if it failed. */
    public int controlTransferResult;

    @Override
    public void close() {
        closed = true;
    }

    @Override
    public int controlTransfer(final int requestType, final int request, final int value, final int index,
                               final byte[] buffer, final int length, final int timeout) {
        return controlTransferResult;
    }

    /** Hands out the oldest queued request as completed, with no input, or null if none is queued. */
    @Override
    public UsbRequest requestWait() {
        return queuedRequests.poll();
    }
}
