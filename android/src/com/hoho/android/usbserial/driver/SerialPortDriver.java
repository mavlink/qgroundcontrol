package com.hoho.android.usbserial.driver;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SerialPortDriver extends CommonUsbSerialDriver {
    public static final int DEVICE_ID = 0x4004;
    public static final String DEVICE_NAME = "/dev/ttyHSL2";
    public static final String MANUFACTURER = "GT";
    private static final String TAG = "SerialPort";

    private SerialPort mSerialPort = null;
    private OutputStream out;
    private InputStream in;

    public SerialPortDriver(UsbDevice usbDevice) {
        super(usbDevice);
    }

    @Override
    public void open() throws IOException{
        if (mSerialPort == null) {
            mSerialPort = new SerialPort(new File(DEVICE_NAME), 115200, 0);
        }
        
        out = mSerialPort.getOutputStream();
        in = mSerialPort.getInputStream();
    }

    @Override
    public void close() throws IOException{
        if (mSerialPort != null) {
            mSerialPort.close();
            mSerialPort = null;
        }
    }

    @Override
    public int read(final byte[] dest, final int timeoutMillis) throws IOException{
        int recved = in.read(dest);
        Log.d(TAG, "readDataBlock : " + bytesToHexString(dest, recved));
        return recved;
    }

    @Override
    public int write(final byte[] src, final int timeoutMillis) throws IOException{
        if (out != null) {
            out.write(src);
            // Log.e(TAG, "sendBuffer : "+bytesToHexString(src));
            return src.length;
        }
        return 0;
    }

    @Override
    public int permissionStatus() {
        return UsbSerialDriver.permissionStatusSuccess;
    }

    @Override
    public void setParameters(
            int baudRate, int dataBits, int stopBits, int parity) throws IOException{
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
        return true;
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
        return true;
    }
    
    @Override
    public void setRTS(boolean value) throws IOException {
    }

    /**
     * Convert byte[] to hex string.这里我们可以将byte转换成int，然后利用Integer.toHexString(int)
     *来转换成16进制字符串。
     * @param src byte[] data
     * @return hex string
     */
    public static String bytesToHexString(byte[] src){
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv + " ");
        }
        return stringBuilder.toString();
    }

    public static String bytesToHexString(byte[] src, int size){
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < size; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv + " ");
        }
        return stringBuilder.toString();
    }

}
