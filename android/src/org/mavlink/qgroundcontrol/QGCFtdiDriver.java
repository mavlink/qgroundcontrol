package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;
import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

final class QGCFtdiDriver {
    private static final String TAG = QGCFtdiDriver.class.getSimpleName();

    private static Context sAppContext;
    private static D2xxManager sManager;

    private final FT_Device _device;

    private int _flowControl = UsbSerialPort.FlowControl.NONE.ordinal();
    private boolean _dtr;
    private boolean _rts;

    private QGCFtdiDriver(final FT_Device device) {
        _device = device;
    }

    static void initialize(final Context context) {
        sAppContext = context.getApplicationContext();
        try {
            sManager = D2xxManager.getInstance(sAppContext);
        } catch (D2xxManager.D2xxException e) {
            sManager = null;
            QGCLogger.w(TAG, "D2XX manager unavailable: " + e.getMessage());
        } catch (Throwable t) {
            sManager = null;
            QGCLogger.w(TAG, "D2XX manager unavailable: " + t.getMessage());
        }
    }

    static void cleanup() {
        sManager = null;
        sAppContext = null;
    }

    static boolean isAvailable() {
        return (sManager != null) && (sAppContext != null);
    }

    static boolean isFtdiDevice(final UsbDevice device) {
        return device != null
            && device.getVendorId() == QGCUsbId.VENDOR_FTDI
            && QGCUsbId.isSupportedFtdiProductId(device.getProductId());
    }

    static QGCFtdiDriver open(final UsbDevice device) {
        if (sManager == null || sAppContext == null || !isFtdiDevice(device)) {
            return null;
        }

        try {
            final FT_Device d2xxDevice = sManager.openByUsbDevice(sAppContext, device);
            if (d2xxDevice == null || !d2xxDevice.isOpen()) {
                return null;
            }
            return new QGCFtdiDriver(d2xxDevice);
        } catch (Throwable t) {
            QGCLogger.w(TAG, "Failed to open D2XX FTDI device " + device.getDeviceName() + ": " + t.getMessage());
            return null;
        }
    }

    boolean isOpen() {
        return _device != null && _device.isOpen();
    }

    void close() {
        try {
            if (_device != null) {
                _device.close();
            }
        } catch (Throwable t) {
            QGCLogger.w(TAG, "Error closing D2XX device: " + t.getMessage());
        }
    }

    int write(final byte[] data, final int length, final int timeoutMSec) {
        if (!isOpen() || data == null || length <= 0) {
            return -1;
        }

        final int writeLength = Math.min(length, data.length);
        final byte[] writeData = (writeLength == data.length) ? data : Arrays.copyOf(data, writeLength);
        try {
            return _device.write(writeData, writeLength, true, timeoutMSec);
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error writing D2XX data", t);
            return -1;
        }
    }

    byte[] read(final int length, final int timeoutMs) {
        if (!isOpen() || length <= 0) {
            return new byte[]{};
        }

        final byte[] buffer = new byte[length];
        try {
            final int bytesRead = _device.read(buffer, length, timeoutMs);
            if (bytesRead <= 0) {
                return new byte[]{};
            }
            return (bytesRead < length) ? Arrays.copyOf(buffer, bytesRead) : buffer;
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error reading D2XX data", t);
            return new byte[]{};
        }
    }

    boolean setParameters(final int baudRate, final int dataBits, final int stopBits, final int parity) {
        if (!isOpen()) {
            return false;
        }
        try {
            final boolean baudOk = _device.setBaudRate(baudRate);
            final boolean charsOk = _device.setDataCharacteristics(toD2xxDataBits(dataBits), toD2xxStopBits(stopBits), toD2xxParity(parity));
            return baudOk && charsOk;
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error setting D2XX parameters", t);
            return false;
        }
    }

    boolean getControlLine(final UsbSerialPort.ControlLine controlLine) {
        if (!isOpen()) {
            return false;
        }

        try {
            final short modemStatus = _device.getModemStatus();
            switch (controlLine) {
                case CD:
                    return (modemStatus & D2xxManager.FT_DCD) != 0;
                case CTS:
                    return (modemStatus & D2xxManager.FT_CTS) != 0;
                case DSR:
                    return (modemStatus & D2xxManager.FT_DSR) != 0;
                case DTR:
                    return _dtr;
                case RI:
                    return (modemStatus & D2xxManager.FT_RI) != 0;
                case RTS:
                    return _rts;
                default:
                    return false;
            }
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error reading D2XX control line " + controlLine, t);
            return false;
        }
    }

    boolean setControlLine(final UsbSerialPort.ControlLine controlLine, final boolean on) {
        if (!isOpen()) {
            return false;
        }

        try {
            switch (controlLine) {
                case DTR:
                    _dtr = on;
                    return on ? _device.setDtr() : _device.clrDtr();
                case RTS:
                    _rts = on;
                    return on ? _device.setRts() : _device.clrRts();
                default:
                    return false;
            }
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error setting D2XX control line " + controlLine, t);
            return false;
        }
    }

    int[] getControlLines() {
        final List<Integer> lines = new ArrayList<>();
        if (getControlLine(UsbSerialPort.ControlLine.RTS)) {
            lines.add(UsbSerialPort.ControlLine.RTS.ordinal());
        }
        if (getControlLine(UsbSerialPort.ControlLine.CTS)) {
            lines.add(UsbSerialPort.ControlLine.CTS.ordinal());
        }
        if (getControlLine(UsbSerialPort.ControlLine.DTR)) {
            lines.add(UsbSerialPort.ControlLine.DTR.ordinal());
        }
        if (getControlLine(UsbSerialPort.ControlLine.DSR)) {
            lines.add(UsbSerialPort.ControlLine.DSR.ordinal());
        }
        if (getControlLine(UsbSerialPort.ControlLine.CD)) {
            lines.add(UsbSerialPort.ControlLine.CD.ordinal());
        }
        if (getControlLine(UsbSerialPort.ControlLine.RI)) {
            lines.add(UsbSerialPort.ControlLine.RI.ordinal());
        }
        return lines.stream().mapToInt(Integer::intValue).toArray();
    }

    int getFlowControl() {
        return _flowControl;
    }

    boolean setFlowControl(final int flowControl) {
        if (!isOpen()) {
            return false;
        }
        try {
            final short flowMode = toD2xxFlowControl(flowControl);
            final boolean result = _device.setFlowControl(flowMode, (byte) 0x11, (byte) 0x13);
            if (result) {
                _flowControl = flowControl;
            }
            return result;
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error setting D2XX flow control", t);
            return false;
        }
    }

    boolean setBreak(final boolean on) {
        if (!isOpen()) {
            return false;
        }
        try {
            return on ? _device.setBreakOn() : _device.setBreakOff();
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error setting D2XX break condition", t);
            return false;
        }
    }

    boolean purgeBuffers(final boolean input, final boolean output) {
        if (!isOpen()) {
            return false;
        }

        byte purgeFlags = 0;
        if (input) {
            purgeFlags |= D2xxManager.FT_PURGE_RX;
        }
        if (output) {
            purgeFlags |= D2xxManager.FT_PURGE_TX;
        }
        if (purgeFlags == 0) {
            return true;
        }

        try {
            return _device.purge(purgeFlags);
        } catch (Throwable t) {
            QGCLogger.e(TAG, "Error purging D2XX buffers", t);
            return false;
        }
    }

    private static byte toD2xxDataBits(final int dataBits) {
        return (dataBits == 7) ? D2xxManager.FT_DATA_BITS_7 : D2xxManager.FT_DATA_BITS_8;
    }

    private static byte toD2xxStopBits(final int stopBits) {
        if (stopBits == UsbSerialPort.STOPBITS_2) {
            return D2xxManager.FT_STOP_BITS_2;
        }
        return D2xxManager.FT_STOP_BITS_1;
    }

    private static byte toD2xxParity(final int parity) {
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

    private static short toD2xxFlowControl(final int flowControl) {
        if (flowControl == UsbSerialPort.FlowControl.RTS_CTS.ordinal()) {
            return D2xxManager.FT_FLOW_RTS_CTS;
        }
        if (flowControl == UsbSerialPort.FlowControl.DTR_DSR.ordinal()) {
            return D2xxManager.FT_FLOW_DTR_DSR;
        }
        if (flowControl == UsbSerialPort.FlowControl.XON_XOFF.ordinal()) {
            return D2xxManager.FT_FLOW_XON_XOFF;
        }
        return D2xxManager.FT_FLOW_NONE;
    }
}
