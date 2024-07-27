package org.mavlink.qgroundcontrol;

import org.mavlink.qgroundcontrol.QGCUsbId;

import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;

class QGCProber
{
    static UsbSerialProber getQGCProber()
    {
        final ProbeTable customTable = new ProbeTable();

        return new UsbSerialProber(customTable);
    }
}
