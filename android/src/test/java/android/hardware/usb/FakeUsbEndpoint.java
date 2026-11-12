package android.hardware.usb;

/** A USB endpoint for JVM unit tests. It lives in this package because UsbEndpoint has no public constructor. */
public final class FakeUsbEndpoint extends UsbEndpoint {
    @Override
    public int getMaxPacketSize() {
        return 64;
    }
}
