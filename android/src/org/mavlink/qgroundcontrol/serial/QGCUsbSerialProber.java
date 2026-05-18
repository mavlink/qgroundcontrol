package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDevice;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;

/**
 * Builds the {@link UsbSerialProber} used by {@link QGCUsbSerialManager}.
 *
 * <p>Driver matching is delegated to mik3y's default probe table — class-descriptor
 * probing for CDC-ACM, plus the library's built-in VID/PID lists for CP210x / FTDI /
 * CH34x / Prolific. The only QGC-specific behavior is preferring {@link QGCFtdiSerialDriver}
 * (D2XX-backed) when D2XX actually enumerates the attached FTDI device. D2XX writes are
 * routed through {@link AsyncUsbWritePump#forD2xx} (single worker, baud-throttled,
 * {@code FT_Device.write(wait=true)}) so the chip's TX FIFO is never overrun.
 */
public class QGCUsbSerialProber {

    private static final String TAG = QGCUsbSerialProber.class.getSimpleName();

    public static UsbSerialProber getQGCUsbSerialProber() {
        return new UsbSerialProber(UsbSerialProber.getDefaultProbeTable()) {
            @Override
            public UsbSerialDriver probeDevice(final UsbDevice device) {
                if (device == null) return null;
                final String tag = String.format("vid=0x%04X pid=0x%04X (%s)",
                        device.getVendorId(), device.getProductId(), device.getDeviceName());
                // QGC issue #14146: canOpenViaD2XX returns true on Android 14/OneUI 6
                // when the subsequent D2XX open will actually fail. Gate the entire D2XX
                // path off until hardware validation is done; VCP-mode FtdiSerialDriver
                // covers the same chips with no functional gap for QGC's use case.
                if (QGCFtdiSerialDriver.ENABLE_D2XX) {
                    final boolean d2xxAvail = QGCFtdiSerialDriver.isAvailable();
                    final boolean isFtdi = QGCFtdiSerialDriver.isFtdiDevice(device);
                    if (d2xxAvail && isFtdi && QGCFtdiSerialDriver.canOpenViaD2XX(device)) {
                        QGCLogger.i(TAG, "probe " + tag + " -> QGCFtdiSerialDriver (D2XX)");
                        return new QGCFtdiSerialDriver(device);
                    }
                }
                final UsbSerialDriver driver = super.probeDevice(device);
                QGCLogger.i(TAG, "probe " + tag
                        + " -> " + (driver == null ? "no driver" : driver.getClass().getSimpleName()));
                return driver;
            }
        };
    }
}
