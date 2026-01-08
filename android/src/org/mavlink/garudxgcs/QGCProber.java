package org.mavlink.garudxgcs;

import org.mavlink.garudxgcs.QGCUsbId;

import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;

public class QGCProber
{
    public static UsbSerialProber getQGCProber() {
        return new UsbSerialProber(getQGCProbeTable());
    }

    public static ProbeTable getQGCProbeTable() {
        final ProbeTable probeTable = new ProbeTable();

        return probeTable;
    }
}
