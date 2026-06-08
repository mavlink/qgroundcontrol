package android.hardware.usb;

public final class FakeUsbDevice extends UsbDevice {

    private final int interfaceCount;
    private final UsbInterface[] interfaces;

    public FakeUsbDevice(final int interfaceCount) {
        this.interfaceCount = interfaceCount;
        this.interfaces = new UsbInterface[Math.max(interfaceCount, 0)];
    }

    @Override
    public int getInterfaceCount() {
        return interfaceCount;
    }

    @Override
    public UsbInterface getInterface(final int index) {
        return interfaces[index];
    }

    @Override
    public int getDeviceId() {
        return 0;
    }
}
