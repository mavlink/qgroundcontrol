package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import com.hoho.android.usbserial.driver.UsbSerialDriver;

import java.util.List;

/** What the serial classes need from Android's USB host API. Unit tests supply a fake. */
interface QGCUsbSerialHost {
    List<UsbSerialDriver> findDrivers();

    /**
     * Returns a driver for the device that nothing else has, or null if no driver knows the device. Asks neither
     * the device nor Android's USB service.
     */
    UsbSerialDriver probeDriver(UsbDevice device);

    boolean hasPermission(UsbDevice device);

    void requestPermission(UsbDevice device);

    /** Returns null if the device cannot be opened. */
    UsbDeviceConnection openDevice(UsbDevice device);

    /**
     * Returns whether the user would get a permission request as the only dialog: QGC's window has the focus,
     * so QGC is in front and no dialog of the system is on top of it, such as Android's own prompt for an
     * attached device or an earlier request.
     */
    boolean canAskUser();
}
