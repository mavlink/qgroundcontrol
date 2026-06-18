package org.mavlink.qgroundcontrol.serial;

import org.qtproject.qt.android.UsedFromNativeCode;

/** Structured device-info record for one USB serial port; {@link QGCUsbSerialManager#packPortsInfo} serialises an array of these into the JSON wire format that C++ {@code SerialPortInfoCodec} decodes (no per-field JNI getField). */
@UsedFromNativeCode
public record UsbPortInfo(
        String deviceName,
        String productName,
        String manufacturerName,
        String serialNumber,
        int productId,
        int vendorId,
        int[] supportedBaudRates) {
}
