package org.mavlink.qgroundcontrol.serial;

import com.hoho.android.usbserial.driver.FtdiSerialDriver;
import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;

final class QGCUsbSerialProber {
    private static final int VENDOR_FTDI = 0x0403;
    private static final int DEVICE_FTDI_FT4232HA = 0x6048;

    /**
     * The FTDI products that the library's table lacks, as {vendor ID, product ID}. Android asks the user about
     * a newly attached device only if res/xml/device_filter.xml lists it, so a product added here goes there too.
     */
    static final int[][] ADDED_FTDI_PRODUCTS = {
        // Linux's ftdi_sio drives it exactly like the FT4232H.
        {VENDOR_FTDI, DEVICE_FTDI_FT4232HA},
    };

    private QGCUsbSerialProber() {}

    static UsbSerialProber getQGCUsbSerialProber() {
        return new UsbSerialProber(getQGCProbeTable());
    }

    /**
     * The library's table, plus the devices it does not know. Flight controllers and GPS receivers need no entry:
     * they are CDC-ACM devices, which the library recognises by their interfaces, whatever their IDs.
     */
    private static ProbeTable getQGCProbeTable() {
        final ProbeTable probeTable = UsbSerialProber.getDefaultProbeTable();

        for (final int[] product : ADDED_FTDI_PRODUCTS) {
            probeTable.addProduct(product[0], product[1], FtdiSerialDriver.class);
        }

        return probeTable;
    }
}
