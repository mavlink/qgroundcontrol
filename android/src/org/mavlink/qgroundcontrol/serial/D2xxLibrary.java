package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.content.Context;
import android.hardware.usb.UsbDevice;

import com.ftdi.j2xx.D2xxManager;

import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.util.concurrent.atomic.AtomicReference;

/**
 * D2XX runtime plumbing: library lifecycle, capability checks (isAvailable, isFtdiDevice, canOpenViaD2XX),
 * and the helpers that translate {@link UsbSerialPort} enums into FTDI D2XX byte/short constants. All state is process-global.
 */
final class D2xxLibrary {

    private static final String TAG = D2xxLibrary.class.getSimpleName();

    static final int VENDOR_FTDI = 0x0403;

    static final byte D2XX_LATENCY_TIMER_MS = 2;
    static final int D2XX_READ_TIMEOUT_MS = 50;

    // D2XX defaults to 16×16 KB = 256 KB/port; SIOManager only submits READ_BUF_SIZE (2 KB) at a time, so trim to 4×4 KB = 16 KB (valid: bufferNumber 2-16, maxBufferSize 64-16384).
    static final int D2XX_BUFFER_NUMBER = 4;
    static final int D2XX_MAX_BUFFER_SIZE = 4096;

    /** D2XX manager + the Context it was initialised against, published together via {@link #sD2xx} so an open() snapshot sees a consistent pair even if cleanup() races. */
    static final class Handle {
        final Context appContext;
        final D2xxManager manager;
        Handle(final Context appContext, final D2xxManager manager) {
            this.appContext = appContext;
            this.manager = manager;
        }
    }

    private static final AtomicReference<Handle> sD2xx = new AtomicReference<>();

    private D2xxLibrary() {}

    static void initialize(final Context context) {
        final Context appContext = context.getApplicationContext();
        D2xxManager manager = null;
        try {
            manager = D2xxManager.getInstance(appContext);
            // Disable D2XX's internal permission + broadcast handling — QGC owns both (QGCUsbSerialManager / UsbAttachDetachReceiver).
            // Running D2XX's parallel handlers creates two competing flows for the same USB events (likely root of #14146; see TN_147).
            manager.setRequestPermission(false);
            manager.setUsbRegisterBroadcast(false);
        } catch (D2xxManager.D2xxException e) {
            QGCLogger.w(TAG, "D2XX manager unavailable (D2xxException): " + e.getMessage());
        } catch (Throwable t) {
            QGCLogger.e(TAG, "D2XX manager unavailable (" + t.getClass().getName() + ")", t);
        }
        sD2xx.set((manager != null) ? new Handle(appContext, manager) : null);
        String libVersion = "unknown";
        if (manager != null) {
            try {
                libVersion = "0x" + Integer.toHexString(D2xxManager.getLibraryVersion());
            } catch (Throwable t) {
                libVersion = "unavailable";
            }
        }
        QGCLogger.i(TAG, "D2XX initialize: manager=" + (manager != null ? "ready" : "null")
                + " libVersion=" + libVersion);
    }

    static void cleanup() {
        sD2xx.set(null);
    }

    static boolean isAvailable() {
        return sD2xx.get() != null;
    }

    /** Atomic snapshot of the (manager, context) pair, or null if uninitialised. */
    static Handle handle() {
        return sD2xx.get();
    }

    // Cheap VID pre-gate; canOpenViaD2XX()'s D2xxManager.isFtDevice() is the PID authority (no parallel list to drift).
    static boolean isFtdiDevice(final UsbDevice device) {
        return device != null && device.getVendorId() == VENDOR_FTDI;
    }

    static int d2xxLocation(final int physicalDeviceId, final int interfaceId) {
        if (interfaceId < 0 || interfaceId > 3) {
            throw new IllegalArgumentException("interfaceId must be 0-3, got " + interfaceId);
        }
        return (physicalDeviceId << 4) | ((interfaceId + 1) & 0x0f);
    }

    /** True only if the D2XX library currently enumerates {@code device}; generic FT232R USB-TTL adapters present in VCP mode and fall through to the stock {@code FtdiSerialDriver}. */
    static boolean canOpenViaD2XX(final UsbDevice device) {
        if (!isFtdiDevice(device)) return false;
        final Handle h = sD2xx.get();
        if (h == null) {
            QGCLogger.d(TAG, "canOpenViaD2XX: D2XX handle null");
            return false;
        }
        // D2xxManager.isFtDevice is the canonical pre-open check; the prior createDeviceInfoList + serial-match heuristic could return true for devices D2XX would then reject (root of #14146).
        try {
            return h.manager.isFtDevice(device);
        } catch (Throwable t) {
            QGCLogger.e(TAG, "canOpenViaD2XX check failed (" + t.getClass().getName() + ")", t);
            return false;
        }
    }

    static D2xxManager.DriverParameters makeDriverParameters() {
        final D2xxManager.DriverParameters params = new D2xxManager.DriverParameters();
        params.setReadTimeout(D2XX_READ_TIMEOUT_MS);
        params.setBufferNumber(D2XX_BUFFER_NUMBER);
        params.setMaxBufferSize(D2XX_MAX_BUFFER_SIZE);
        return params;
    }

    static int normalizeReadResult(final int bytesRead, final int requestedLength) throws java.io.IOException {
        if (bytesRead < 0) {
            throw new java.io.IOException("D2XX read returned " + bytesRead);
        }
        return Math.min(bytesRead, requestedLength);
    }

    static byte toD2xxDataBits(final int dataBits) throws java.io.IOException {
        // D2XX only exposes 7/8 data bits (D2xxManager defines no FT_DATA_BITS_5/6); reject rather than silently send 8.
        switch (dataBits) {
            case 7:
                return D2xxManager.FT_DATA_BITS_7;
            case 8:
                return D2xxManager.FT_DATA_BITS_8;
            default:
                throw new java.io.IOException("D2XX supports only 7 or 8 data bits, requested " + dataBits);
        }
    }

    static byte toD2xxStopBits(final int stopBits) {
        if (stopBits == UsbSerialPort.STOPBITS_2) {
            return D2xxManager.FT_STOP_BITS_2;
        }
        return D2xxManager.FT_STOP_BITS_1;
    }

    static byte toD2xxParity(final int parity) {
        switch (parity) {
            case UsbSerialPort.PARITY_ODD:
                return D2xxManager.FT_PARITY_ODD;
            case UsbSerialPort.PARITY_EVEN:
                return D2xxManager.FT_PARITY_EVEN;
            case UsbSerialPort.PARITY_MARK:
                return D2xxManager.FT_PARITY_MARK;
            case UsbSerialPort.PARITY_SPACE:
                return D2xxManager.FT_PARITY_SPACE;
            case UsbSerialPort.PARITY_NONE:
            default:
                return D2xxManager.FT_PARITY_NONE;
        }
    }

    static short toD2xxFlowControl(final UsbSerialPort.FlowControl flowControl) {
        if (flowControl == UsbSerialPort.FlowControl.RTS_CTS) {
            return D2xxManager.FT_FLOW_RTS_CTS;
        }
        if (flowControl == UsbSerialPort.FlowControl.DTR_DSR) {
            return D2xxManager.FT_FLOW_DTR_DSR;
        }
        if (flowControl == UsbSerialPort.FlowControl.XON_XOFF) {
            return D2xxManager.FT_FLOW_XON_XOFF;
        }
        return D2xxManager.FT_FLOW_NONE;
    }
}
