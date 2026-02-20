package org.mavlink.qgroundcontrol;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;

public class QGCUsbSerialProber {

    public static UsbSerialProber getQGCUsbSerialProber() {
        return new UsbSerialProber(getQGCProbeTable());
    }

    public static ProbeTable getQGCProbeTable() {
        final ProbeTable probeTable = UsbSerialProber.getDefaultProbeTable();

        // FTDI (D2XX-backed adapter). When unavailable, keep default FTDI probing.
        if (QGCFtdiDriver.isAvailable()) {
            probeTable.addProduct(QGCUsbId.VENDOR_FTDI, QGCUsbId.DEVICE_FTDI_FT232R, QGCFtdiSerialDriver.class);
            probeTable.addProduct(QGCUsbId.VENDOR_FTDI, QGCUsbId.DEVICE_FTDI_FT2232H, QGCFtdiSerialDriver.class);
            probeTable.addProduct(QGCUsbId.VENDOR_FTDI, QGCUsbId.DEVICE_FTDI_FT4232H, QGCFtdiSerialDriver.class);
            probeTable.addProduct(QGCUsbId.VENDOR_FTDI, QGCUsbId.DEVICE_FTDI_FT232H, QGCFtdiSerialDriver.class);
            probeTable.addProduct(QGCUsbId.VENDOR_FTDI, QGCUsbId.DEVICE_FTDI_FT231X, QGCFtdiSerialDriver.class);
        }

        // PX4
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V1, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V2, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V4, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V4PRO, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V5, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V5X, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V6C, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V6U, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V6X, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU_V6XRT, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4MINDPX_V2, CdcAcmSerialDriver.class);

        // u-blox GPS
        probeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_5, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_6, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_7, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_8, CdcAcmSerialDriver.class);

        // OpenPilot
        probeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_REVOLUTION, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_OPLINK, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_SPARKY2, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_CC3D, CdcAcmSerialDriver.class);

        // ArduPilot
        probeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS2, CdcAcmSerialDriver.class);

        // DragonLink
        probeTable.addProduct(QGCUsbId.VENDOR_DRAGONLINK, QGCUsbId.DEVICE_DRAGONLINK, CdcAcmSerialDriver.class);

        // CubePilot
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_BLACK, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_BLACK_BOOTLOADER, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_ORANGE, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_ORANGE2, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_ORANGEPLUS, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_YELLOW_BOOTLOADER, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_YELLOW, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_PURPLE_BOOTLOADER, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_CUBEPILOT, QGCUsbId.DEVICE_CUBE_PURPLE, CdcAcmSerialDriver.class);

        // CUAV
        probeTable.addProduct(QGCUsbId.VENDOR_CUAV, QGCUsbId.DEVICE_CUAV_NORA, CdcAcmSerialDriver.class);

        // Holybro
        probeTable.addProduct(QGCUsbId.VENDOR_HOLYBRO, QGCUsbId.DEVICE_PIXHAWK4, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_HOLYBRO, QGCUsbId.DEVICE_PH4_MINI, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_HOLYBRO, QGCUsbId.DEVICE_DURANDAL, CdcAcmSerialDriver.class);

        // Laser Navigation (VRBrain)
        probeTable.addProduct(QGCUsbId.VENDOR_LASER_NAVIGATION, QGCUsbId.DEVICE_VRBRAIN_V51, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_LASER_NAVIGATION, QGCUsbId.DEVICE_VRBRAIN_V52, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_LASER_NAVIGATION, QGCUsbId.DEVICE_VRBRAIN_V54, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_LASER_NAVIGATION, QGCUsbId.DEVICE_VRCORE_V10, CdcAcmSerialDriver.class);
        probeTable.addProduct(QGCUsbId.VENDOR_LASER_NAVIGATION, QGCUsbId.DEVICE_VRUBRAIN_V51, CdcAcmSerialDriver.class);

        return probeTable;
    }
}
