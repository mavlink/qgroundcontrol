package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.util.Locale;

/**
 * The name of a port of a USB serial device, as the name of the device and the index of the port on it.
 *
 * <p>A port is named by its USB device name, plus "#p&lt;index&gt;" for each port of a multi-port device.
 * Android gives a device a new name each time it is attached, so a port also has a stable name, built from
 * the device's IDs and serial number; either one opens it.
 */
record QGCUsbSerialPortName(String deviceName, int portIndex) {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialPortName";
    private static final String PORT_SUFFIX = "#p";
    // C++ tells a stable name by this prefix too, in qserialportinfo_android.cpp.
    private static final String STABLE_NAME_PREFIX = "usb:";

    /** A name with no valid port suffix is taken whole as the device name, with port 0. */
    static QGCUsbSerialPortName parse(final String portName) {
        if (portName == null) {
            return new QGCUsbSerialPortName("", 0);
        }

        final int split = portName.lastIndexOf(PORT_SUFFIX);
        if (split <= 0) {
            return new QGCUsbSerialPortName(portName, 0);
        }

        try {
            final int portIndex = Integer.parseInt(portName.substring(split + PORT_SUFFIX.length()));
            return new QGCUsbSerialPortName(portName.substring(0, split), Math.max(0, portIndex));
        } catch (final NumberFormatException e) {
            QGCLogger.w(TAG, "Invalid port suffix in " + portName + ", using port 0");
            return new QGCUsbSerialPortName(portName, 0);
        }
    }

    /** Returns the name of the port, given how many ports its device has. */
    String format(final int portCount) {
        if (portCount <= 1 && portIndex == 0) {
            return deviceName;
        }
        return deviceName + PORT_SUFFIX + portIndex;
    }

    /**
     * Returns a name that still identifies the device after it is replugged:
     * "usb:&lt;vendor&gt;:&lt;product&gt;:&lt;serial&gt;", with the IDs as four hex digits. Devices of a kind
     * that report the same serial number, or none, share a name, which opens no port while more than one of them
     * is attached.
     *
     * <p>A serial number is whatever text its device reports, and may itself end like a port suffix. Its '#' is
     * therefore written as "%23", and '%' as "%25" to keep the names distinct, so that the only '#' in the name
     * of a port starts its suffix. Whitespace is written in the same way, a space as "%20": a link's port name
     * loses the whitespace at its ends when it is stored, and would then name no device, or another one.
     */
    static String stableDeviceName(final UsbDevice device, final String serialNumber) {
        return String.format(Locale.ROOT, "%s%04x:%04x:%s", STABLE_NAME_PREFIX, device.getVendorId(),
                device.getProductId(), (serialNumber != null) ? escapeSerialNumber(serialNumber) : "");
    }

    private static String escapeSerialNumber(final String serialNumber) {
        final StringBuilder escaped = new StringBuilder(serialNumber.length());
        for (int i = 0; i < serialNumber.length(); i++) {
            final char c = serialNumber.charAt(i);
            if (c != '%' && c != '#' && !Character.isSpaceChar(c) && !Character.isISOControl(c)) {
                escaped.append(c);
            } else {
                // A character beyond two hex digits gets a form that no shorter one followed by digits spells.
                escaped.append(String.format(Locale.ROOT, (c <= 0xFF) ? "%%%02X" : "%%u%04X", (int) c));
            }
        }
        return escaped.toString();
    }

    /** Returns whether the device is named by its stable name. */
    boolean isStable() {
        return deviceName.startsWith(STABLE_NAME_PREFIX);
    }

    /**
     * Returns whether the device is named by a stable name with the vendor and product IDs of the given device,
     * whatever serial number follows them.
     */
    boolean hasIdsOf(final UsbDevice device) {
        return deviceName.startsWith(stableDeviceName(device, null));
    }
}
