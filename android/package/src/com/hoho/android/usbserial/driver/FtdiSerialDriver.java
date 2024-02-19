/* Copyright 2011 Google Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Project home page: http://code.google.com/p/usb-serial-for-android/
 */

// IMPORTANT NOTE:
//  This code has been modified from the original source. It now uses the FTDI driver provided by
//  ftdichip.com to communicate with an FTDI device. The previous code did not work with all FTDI
//  devices.
package com.hoho.android.usbserial.driver;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbRequest;
import android.util.Log;

import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.LinkedHashMap;
import java.util.Map;

import org.mavlink.qgroundcontrol.QGCActivity;

/**
 * A {@link CommonUsbSerialDriver} implementation for a variety of FTDI devices
 * <p>
 * This driver is based on
 * <a href="http://www.intra2net.com/en/developer/libftdi">libftdi</a>, and is
 * copyright and subject to the following terms:
 *
 * <pre>
 *   Copyright (C) 2003 by Intra2net AG
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License
 *   version 2.1 as published by the Free Software Foundation;
 *
 *   opensource@intra2net.com
 *   http://www.intra2net.com/en/developer/libftdi
 * </pre>
 *
 * </p>
 * <p>
 * Some FTDI devices have not been tested; see later listing of supported and
 * unsupported devices. Devices listed as "supported" support the following
 * features:
 * <ul>
 * <li>Read and write of serial data (see {@link #read(byte[], int)} and
 * {@link #write(byte[], int)}.
 * <li>Setting baud rate (see {@link #setBaudRate(int)}).
 * </ul>
 * </p>
 * <p>
 * Supported and tested devices:
 * <ul>
 * <li>{@value DeviceType#TYPE_R}</li>
 * </ul>
 * </p>
 * <p>
 * Unsupported but possibly working devices (please contact the author with
 * feedback or patches):
 * <ul>
 * <li>{@value DeviceType#TYPE_2232C}</li>
 * <li>{@value DeviceType#TYPE_2232H}</li>
 * <li>{@value DeviceType#TYPE_4232H}</li>
 * <li>{@value DeviceType#TYPE_AM}</li>
 * <li>{@value DeviceType#TYPE_BM}</li>
 * </ul>
 * </p>
 *
 * @author mike wakerly (opensource@hoho.com)
 * @see <a href="http://code.google.com/p/usb-serial-for-android/">USB Serial
 * for Android project page</a>
 * @see <a href="http://www.ftdichip.com/">FTDI Homepage</a>
 * @see <a href="http://www.intra2net.com/en/developer/libftdi">libftdi</a>
 */
public class FtdiSerialDriver extends CommonUsbSerialDriver {

    public static final int USB_TYPE_STANDARD = 0x00 << 5;
    public static final int USB_TYPE_CLASS = 0x00 << 5;
    public static final int USB_TYPE_VENDOR = 0x00 << 5;
    public static final int USB_TYPE_RESERVED = 0x00 << 5;

    public static final int USB_RECIP_DEVICE = 0x00;
    public static final int USB_RECIP_INTERFACE = 0x01;
    public static final int USB_RECIP_ENDPOINT = 0x02;
    public static final int USB_RECIP_OTHER = 0x03;

    public static final int USB_ENDPOINT_IN = 0x80;
    public static final int USB_ENDPOINT_OUT = 0x00;

    public static final int USB_WRITE_TIMEOUT_MILLIS = 5000;
    public static final int USB_READ_TIMEOUT_MILLIS = 5000;

    // From ftdi.h
    /**
     * Reset the port.
     */
    private static final int SIO_RESET_REQUEST = 0;

    /**
     * Set the modem control register.
     */
    private static final int SIO_MODEM_CTRL_REQUEST = 1;

    /**
     * Set flow control register.
     */
    private static final int SIO_SET_FLOW_CTRL_REQUEST = 2;

    /**
     * Set baud rate.
     */
    private static final int SIO_SET_BAUD_RATE_REQUEST = 3;

    /**
     * Set the data characteristics of the port.
     */
    private static final int SIO_SET_DATA_REQUEST = 4;

    private static final int SIO_RESET_SIO = 0;
    private static final int SIO_RESET_PURGE_RX = 1;
    private static final int SIO_RESET_PURGE_TX = 2;

    public static final int FTDI_DEVICE_OUT_REQTYPE =
            UsbConstants.USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT;

    public static final int FTDI_DEVICE_IN_REQTYPE =
            UsbConstants.USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN;

    /**
     * Length of the modem status header, transmitted with every read.
     */
    private static final int MODEM_STATUS_HEADER_LENGTH = 2;

    private final String TAG = FtdiSerialDriver.class.getSimpleName();

    private DeviceType mType;

    /**
     * FTDI chip types.
     */
    private static enum DeviceType {
        TYPE_BM, TYPE_AM, TYPE_2232C, TYPE_R, TYPE_2232H, TYPE_4232H;
    }

    private int mInterface = 0; /* INTERFACE_ANY */

    private int mMaxPacketSize = 64; // TODO(mikey): detect

    /**
     * Due to http://b.android.com/28023 , we cannot use UsbRequest async reads
     * since it gives no indication of number of bytes read. Set this to
     * {@code true} on platforms where it is fixed.
     */
    private static final boolean ENABLE_ASYNC_READS = false;

    FT_Device m_ftDev;

    /**
     * Filter FTDI status bytes from buffer
     * @param src The source buffer (which contains status bytes)
     * @param dest The destination buffer to write the status bytes into (can be src)
     * @param totalBytesRead Number of bytes read to src
     * @param maxPacketSize The USB endpoint max packet size
     * @return The number of payload bytes
     */
    private final int filterStatusBytes(byte[] src, byte[] dest, int totalBytesRead, int maxPacketSize) {
        final int packetsCount = totalBytesRead / maxPacketSize + 1;
        for (int packetIdx = 0; packetIdx < packetsCount; ++packetIdx) {
            final int count = (packetIdx == (packetsCount - 1))
                    ? (totalBytesRead % maxPacketSize) - MODEM_STATUS_HEADER_LENGTH
                    : maxPacketSize - MODEM_STATUS_HEADER_LENGTH;
            if (count > 0) {
                System.arraycopy(src,
                        packetIdx * maxPacketSize + MODEM_STATUS_HEADER_LENGTH,
                        dest,
                        packetIdx * (maxPacketSize - MODEM_STATUS_HEADER_LENGTH),
                        count);
            }
        }

      return totalBytesRead - (packetsCount * 2);
    }

    /**
     * Constructor.
     *
     * @param usbDevice the {@link UsbDevice} to use
     * @param usbConnection the {@link UsbDeviceConnection} to use
     * @throws UsbSerialRuntimeException if the given device is incompatible
     *             with this driver
     */
    public FtdiSerialDriver(UsbDevice usbDevice) {
        super(usbDevice);
        mType = null;
    }

    public void reset() throws IOException {
        int result = mConnection.controlTransfer(FTDI_DEVICE_OUT_REQTYPE, SIO_RESET_REQUEST,
                SIO_RESET_SIO, 0 /* index */, null, 0, USB_WRITE_TIMEOUT_MILLIS);
        if (result != 0) {
            throw new IOException("Reset failed: result=" + result);
        }

        // TODO(mikey): autodetect.
        mType = DeviceType.TYPE_R;
    }

    @Override
    public void open() throws IOException {
        D2xxManager ftD2xx = null;
        try {
            ftD2xx = D2xxManager.getInstance(QGCActivity.m_context);
        } catch (D2xxManager.D2xxException ex) {
            QGCActivity.qgcLogDebug("D2xxManager.getInstance threw exception: " + ex.getMessage());
        }

        if (ftD2xx == null) {
            String errMsg = "Unable to retrieve D2xxManager instance.";
            QGCActivity.qgcLogWarning(errMsg);
            throw new IOException(errMsg);
        }
        QGCActivity.qgcLogDebug("Opened D2xxManager");

        int DevCount = ftD2xx.createDeviceInfoList(QGCActivity.m_context);
        QGCActivity.qgcLogDebug("Found " + DevCount + " ftdi devices.");
        if (DevCount < 1) {
            throw new IOException("No FTDI Devices found");
        }

        m_ftDev = null;
        try {
            m_ftDev = ftD2xx.openByIndex(QGCActivity.m_context, 0);
        } catch (NullPointerException e) {
            QGCActivity.qgcLogDebug("ftD2xx.openByIndex exception: " + e.getMessage());
        } finally {
            if (m_ftDev == null) {
                throw new IOException("No FTDI Devices found");
            }
        }
        QGCActivity.qgcLogDebug("Opened FTDI device.");
    }

    @Override
    public void close() {
        if (m_ftDev != null) {
            try {
                m_ftDev.close();
            } catch (Exception e) {
                QGCActivity.qgcLogWarning("close exception: " + e.getMessage());
            }
            m_ftDev = null;
        }
    }

    @Override
    public int read(byte[] dest, int timeoutMillis) throws IOException {
        int totalBytesRead = 0;
        int bytesAvailable = m_ftDev.getQueueStatus();

        if (bytesAvailable > 0) {
            bytesAvailable = Math.min(4096, bytesAvailable);
            try {
                totalBytesRead = m_ftDev.read(dest, bytesAvailable, timeoutMillis);
            } catch (NullPointerException e) {
                final String errorMsg = "Error reading: " + e.getMessage();
                QGCActivity.qgcLogWarning(errorMsg);
                throw new IOException(errorMsg, e);
            }
        }

        return totalBytesRead;
    }

    @Override
    public int write(byte[] src, int timeoutMillis) throws IOException {
        try {
            m_ftDev.write(src);
            return src.length;
        } catch (Exception e) {
            QGCActivity.qgcLogWarning("Error writing: " + e.getMessage());
        }
        return 0;
    }

    private int setBaudRate(int baudRate) throws IOException {
        try {
            m_ftDev.setBaudRate(baudRate);
            return baudRate;
        } catch (Exception e) {
            QGCActivity.qgcLogWarning("Error setting baud rate: " + e.getMessage());
        }
        return 0;
    }

    @Override
    public void setParameters(int baudRate, int dataBits, int stopBits, int parity) throws IOException {
        setBaudRate(baudRate);

        switch (dataBits) {
        case 7:
            dataBits = D2xxManager.FT_DATA_BITS_7;
            break;
        case 8:
        default:
            dataBits = D2xxManager.FT_DATA_BITS_8;
            break;
        }

        switch (stopBits) {
        default:
        case 0:
            stopBits = D2xxManager.FT_STOP_BITS_1;
            break;
        case 1:
            stopBits = D2xxManager.FT_STOP_BITS_2;
            break;
        }

        switch (parity) {
        default:
        case 0:
            parity = D2xxManager.FT_PARITY_NONE;
            break;
        case 1:
            parity = D2xxManager.FT_PARITY_ODD;
            break;
        case 2:
            parity = D2xxManager.FT_PARITY_EVEN;
            break;
        case 3:
            parity = D2xxManager.FT_PARITY_MARK;
            break;
        case 4:
            parity = D2xxManager.FT_PARITY_SPACE;
            break;
        }

        try {
            m_ftDev.setDataCharacteristics((byte)dataBits, (byte)stopBits, (byte)parity);
        } catch (Exception e) {
            QGCActivity.qgcLogWarning("Error setDataCharacteristics: " + e.getMessage());
        }
    }
    @Override
    public boolean getCD() throws IOException {
        return false;
    }

    @Override
    public boolean getCTS() throws IOException {
        return false;
    }

    @Override
    public boolean getDSR() throws IOException {
        return false;
    }

    @Override
    public boolean getDTR() throws IOException {
        return false;
    }

    @Override
    public void setDTR(boolean value) throws IOException {
    }

    @Override
    public boolean getRI() throws IOException {
        return false;
    }

    @Override
    public boolean getRTS() throws IOException {
        return false;
    }

    @Override
    public void setRTS(boolean value) throws IOException {
    }

    @Override
    public boolean purgeHwBuffers(boolean purgeReadBuffers, boolean purgeWriteBuffers) throws IOException {
        if (purgeReadBuffers) {
            try {
                m_ftDev.purge(D2xxManager.FT_PURGE_RX);
            } catch (Exception e) {
                String errMsg = "Error purgeHwBuffers(RX): "+ e.getMessage();
                QGCActivity.qgcLogWarning(errMsg);
                throw new IOException(errMsg);
            }
        }

        if (purgeWriteBuffers) {
            try {
                m_ftDev.purge(D2xxManager.FT_PURGE_TX);
            } catch (Exception e) {
                String errMsg = "Error purgeHwBuffers(TX): " + e.getMessage();
                QGCActivity.qgcLogWarning(errMsg);
                throw new IOException(errMsg);
            }
        }

        return true;
    }

    public static Map<Integer, int[]> getSupportedDevices() {
        final Map<Integer, int[]> supportedDevices = new LinkedHashMap<Integer, int[]>();
        supportedDevices.put(Integer.valueOf(UsbId.VENDOR_FTDI),
                new int[] {
                    UsbId.FTDI_FT232R,
                    UsbId.FTDI_FT231X,                    
                });
        /*
        supportedDevices.put(Integer.valueOf(UsbId.VENDOR_PX4),
                new int[] {
                    UsbId.DEVICE_PX4FMU
                });
        */
        return supportedDevices;
    }

}
