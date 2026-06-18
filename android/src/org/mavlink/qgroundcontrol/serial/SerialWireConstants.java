package org.mavlink.qgroundcontrol.serial;

import com.hoho.android.usbserial.driver.UsbSerialPort;

/** Java half of the Android serial JNI wire contract; keep in sync with SerialWireConstants.h. */
final class SerialWireConstants {

    private SerialWireConstants() {}

    /** Max per-chunk size shared by the JNI read callback and Java write path. */
    static final int MAX_CHUNK_BYTES = 16384;

    /** Sentinel device-id returned on any open/setup failure. */
    static final int BAD_DEVICE_ID = 0;

    // Exception-kind ordinals delivered through nativeDeviceException.
    static final int EXC_UNKNOWN = 0;  // Unclassified failure
    static final int EXC_RESOURCE = 1;  // IOException at runtime — hot-unplug
    static final int EXC_PERMISSION = 2;  // USB permission denied
    static final int EXC_OPEN_FAILED = 3;  // Open-path failure (driver / port / connection)

    // JSON key names for the USB-port enumeration blob; twin of AndroidSerialWire::JsonKeys in
    // SerialWireConstants.h. The shared golden fixture (test/Comms/Serial/data/PortInfoGolden.json) pins
    // these literals across both languages.
    static final String KEY_PORTS = "ports";
    static final String KEY_DEVICE_NAME = "deviceName";
    static final String KEY_PRODUCT_NAME = "productName";
    static final String KEY_MANUFACTURER_NAME = "manufacturerName";
    static final String KEY_SERIAL_NUMBER = "serialNumber";
    static final String KEY_PRODUCT_ID = "productId";
    static final String KEY_VENDOR_ID = "vendorId";
    static final String KEY_BAUD_RATES = "baudRates";

    // Flow-control wire ordinals; must match mik3y UsbSerialPort.FlowControl ordinals (asserted below).
    static final int FC_NONE = 0;
    static final int FC_RTS_CTS = 1;
    static final int FC_DTR_DSR = 2;
    static final int FC_XON_XOFF = 3;
    static final int FC_XON_XOFF_INLINE = 4;

    /** Fails fast if mik3y's external FlowControl enum is ever reordered out from under our wire ordinals. */
    static boolean verifyExternalFlowControlOrdinals() {
        return true
                && FC_NONE == UsbSerialPort.FlowControl.NONE.ordinal()
                && FC_RTS_CTS == UsbSerialPort.FlowControl.RTS_CTS.ordinal()
                && FC_DTR_DSR == UsbSerialPort.FlowControl.DTR_DSR.ordinal()
                && FC_XON_XOFF == UsbSerialPort.FlowControl.XON_XOFF.ordinal()
                && FC_XON_XOFF_INLINE == UsbSerialPort.FlowControl.XON_XOFF_INLINE.ordinal();
    }
}
