package org.mavlink.qgroundcontrol;

import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import org.mavlink.qgroundcontrol.QGCUsbId;

// implements UsbSerialProber?
class QGCProber
{
    static UsbSerialProber getQGCProber()
    {
        final ProbeTable customTable = new ProbeTable();

        return new UsbSerialProber(customTable);
    }
}
