package android.hardware.usb;

/** A USB device for JVM unit tests. It lives in this package because UsbDevice has no public constructor. */
public final class FakeUsbDevice extends UsbDevice {
    public String productName;
    public String serialNumber;
    public int serialNumberReads;
    public int vendorId;
    public int productId;

    private final int _deviceId;
    private final String _deviceName;

    public FakeUsbDevice(final int deviceId, final String deviceName) {
        _deviceId = deviceId;
        _deviceName = deviceName;
    }

    @Override
    public int getDeviceId() {
        return _deviceId;
    }

    @Override
    public String getDeviceName() {
        return _deviceName;
    }

    @Override
    public String getProductName() {
        return productName;
    }

    @Override
    public String getSerialNumber() {
        serialNumberReads++;
        return serialNumber;
    }

    @Override
    public int getVendorId() {
        return vendorId;
    }

    @Override
    public int getProductId() {
        return productId;
    }

    // The stubbed android.jar would otherwise answer false and 0, which breaks collections.
    @Override
    public boolean equals(final Object other) {
        return this == other;
    }

    @Override
    public int hashCode() {
        return System.identityHashCode(this);
    }
}
