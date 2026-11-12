package org.mavlink.qgroundcontrol.serial;

/**
 * A port of an attached USB serial device, as C++ lists it. AndroidSerialJni.cc reads these fields by name, so
 * it has to name and type each one as it is declared here.
 */
public final class QGCUsbSerialPortInfo {
    /** The name that opens the port. Android assigns it, and assigns another when the device is replugged. */
    public final String name;
    /** A name that still opens the port after its device is replugged. Empty until QGC may open the device. */
    public final String stableName;
    public final String productName;
    public final String manufacturerName;
    /** Empty until QGC may open the device, because Android withholds it. */
    public final String serialNumber;
    public final int vendorId;
    public final int productId;
    /** Whether the port's driver has RTS/CTS flow control. A port without it refuses to be set to it. */
    public final boolean hardwareFlowControl;

    /** A device reports a string it does not have as null, where C++ expects an empty one. */
    QGCUsbSerialPortInfo(final String name, final String stableName, final String productName,
                         final String manufacturerName, final String serialNumber, final int vendorId,
                         final int productId, final boolean hardwareFlowControl) {
        this.name = orEmpty(name);
        this.stableName = orEmpty(stableName);
        this.productName = orEmpty(productName);
        this.manufacturerName = orEmpty(manufacturerName);
        this.serialNumber = orEmpty(serialNumber);
        this.vendorId = vendorId;
        this.productId = productId;
        this.hardwareFlowControl = hardwareFlowControl;
    }

    private static String orEmpty(final String deviceString) {
        return (deviceString != null) ? deviceString : "";
    }
}
