package org.mavlink.qgroundcontrol;

/* Copyright 2013 Google Inc.
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
///////////////////////////////////////////////////////////////////////////////////////////
//  Written by: Mike Goza April 2014
//
//  These routines interface with the Android USB Host devices for serial port communication.
//  The code uses the usb-serial-for-android software library.  The QGCActivity class is the
//  interface to the C++ routines through jni calls.  Do not change the functions without also
//  changing the corresponding calls in the C++ routines or you will break the interface.
//
////////////////////////////////////////////////////////////////////////////////////////////

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.WindowManager;

import com.hoho.android.usbserial.driver.*;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity
{
    /* General */
    private static final String TAG = getSimpleName();
    private static BroadcastReceiver _receiver;

    /* Multicasting */
    private static WifiManager.MulticastLock _wifiMulticastLock;

    /* USB */
    private enum UsbPermission { Unknown, Requested, Granted, Denied }
    private static final int BAD_DEVICE_ID = 0;
    private static final String ACTION_USB_PERMISSION = getPackageName() + "action.USB_PERMISSION";
    private static UsbManager _usbManager;
    private static List<UsbSerialDriver> _usbDrivers;
    private static List<SerialInputOutputManager> _usbIoManagers;
    private static UsbSerialProber _usbProber;
    private static ProbeTable _usbProbeTable;
    private UsbPermission _usbPermission = UsbPermission.Unknown;

    private static native void nativeDeviceNewData(int deviceId, byte[] data);
    private static native void nativeUpdateAvailableJoysticks();
    public static native void nativeLogDebug(String message);
    public static native void nativeLogWarning(String message);

    public native void nativeInit();

    // QGCActivity singleton
    public QGCActivity()
    {

    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Workaround for QTBUG-73138
        WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        _wifiMulticastLock = wifi.createMulticastLock(TAG);
        if(_wifiMulticastLock != null)
        {
            _wifiMulticastLock.setReferenceCounted(true);
            _wifiMulticastLock.acquire();
            Log.d(TAG, "WifiMulticastLock: " + _wifiMulticastLock.toString());
        }
        else
        {
            Log.i(TAG, "WifiMulticastLock not acquired");
        }

        _receiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                String action = intent.getAction();
                Log.i(TAG, "BroadcastReceiver action " + action);

                if(action.equals(ACTION_USB_PERMISSION))
                {
                    _handleUsbPermissions(context, intent);
                }
                else if(action.equals(UsbManager.ACTION_USB_DEVICE_DETACHED))
                {
                    _handleUsbDetached(context, intent);
                }
                else if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED))
                {
                    _handleUsbAttached(context, intent);
                }
                else if(action.equals(BluetoothDevice.ACTION_ACL_CONNECTED))
                {
                    _handleBluetoothConnected(intent);
                }
                else if(action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED))
                {
                    _handleBluetoothDisconnected(intent);
                }

                /* nativeUpdateAvailableJoysticks(); */
            }
        };

        IntentFilter bluetoothFilter = new IntentFilter();
        bluetoothFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        bluetoothFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        registerReceiver(_receiver, bluetoothFilter);

        IntentFilter usbFilter = new IntentFilter();
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        usbFilter.addAction(ACTION_USB_PERMISSION);
        registerReceiver(_receiver, usbFilter);

        _usbDrivers = new ArrayList<UsbSerialDriver>();
        _usbIoManagers = new ArrayList<UsbIoManager>();

        _usbProbeTable = getDefaultProbeTable();
        _addUsbIdsToProbeTable();
        _usbProber = new UsbSerialProber(_usbProbeTable);

        _usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        nativeInit();
    }

    @Override
    protected void onDestroy()
    {
        try
        {
            if (_wifiMulticastLock != null)
            {
                _wifiMulticastLock.release();
                Log.d(TAG, "WifiMulticastLock released.");
            }
        }
        catch(Exception ex)
        {
           Log.e(TAG, "Exception onDestroy WifiMulticastLock: " + ex.getMessage());
        }

        super.onDestroy();
    }

    public void onInit(int status)
    {

    }

    public static String getSDCardPath()
    {
        String result = "";

        StorageManager storageManager = (StorageManager)getSystemService(Activity.STORAGE_SERVICE);
        List<StorageVolume> volumes = storageManager.getStorageVolumes();
        for (StorageVolume vol : volumes)
        {
            Method mMethodGetPath;
            String path;

            try
            {
                mMethodGetPath = vol.getClass().getMethod("getPath");
            }
            catch (NoSuchMethodException e)
            {
                Log.e(TAG, "Exception getSDCardPath mMethodGetPath: " + ex.getMessage());
                e.printStackTrace();
                continue;
            }

            try
            {
                path = (String) mMethodGetPath.invoke(vol);
            }
            catch (Exception ex)
            {
                Log.e(TAG, "Exception getSDCardPath path: " + ex.getMessage());
                ex.printStackTrace();
                continue;
            }

            if (vol.isRemovable())
            {
                Log.i(TAG, "removable sd card mounted " + path);
                result = path;
                break;
            }
            else
            {
                Log.i(TAG, "storage mounted " + path);
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Bluetooth Private Helpers
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    private static void _handleBluetoothConnected(Intent intent)
    {
        Log.i(TAG, "_handleBluetoothConnected");
    }

    private static void _handleBluetoothDisconnected(Intent intent)
    {
        Log.i(TAG, "_handleBluetoothDisconnected");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // USB Private Helpers
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    private static void _addUsbIdsToProbeTable()
    {
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ATMEL, QGCUsbId.DEVICE_ATMEL_LUFA_CDC_DEMO_APP, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_UNO, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_2560, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_SERIAL_ADAPTER, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_ADK, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_2560_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_UNO_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_ADK_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_SERIAL_ADAPTER_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_LEONARDO, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_VAN_OOIJEN_TECH, QGCUsbId.DEVICE_VAN_OOIJEN_TECH_TEENSYDUINO_SERIAL, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_LEAFLABS, QGCUsbId.DEVICE_LEAFLABS_MAPLE, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_5, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_6, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_7, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_8, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_REVOLUTION, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_OPLINK, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_SPARKY2, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_CC3D, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT_CHIBIOS1, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT_CHIBIOS2, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_DRAGONLINK, QGCUsbId.DEVICE_DRAGONLINK, CdcAcmSerialDriver.class);
    }

    private static void _handleUsbPermissions(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
            {
                _usbPermission = UsbPermission.Granted;
                nativeLogDebug("Permission granted to " + device.getDeviceName());
            }
            else
            {
                _usbPermission = UsbPermission.Denied;
                nativeLogDebug("Permission denied for " + device.getDeviceName());
            }
        }
    }

    private static void _handleUsbAttached(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            UsbSerialDriver driver = _usbProber.probeDevice(device);
            UsbDeviceConnection usbConnection = _usbManager.openDevice(device);
            if(usbConnection == null)
            {
                if(usbPermission == UsbPermission.Unknown && !_usbManager.hasPermission(device))
                {
                    final int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_MUTABLE : 0;
                    Intent intent = new Intent(INTENT_ACTION_GRANT_USB);
                    intent.setPackage(context.getPackageName());
                    PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(context, 0, intent, flags);
                    _usbManager.requestPermission(device, usbPermissionIntent);
                    _usbPermission = UsbPermission.Requested;
                }
                else
                {
                    if (!_usbManager.hasPermission(driver.getDevice()))
                    {
                        nativeLogDebug("connection failed: permission denied");
                    }
                    else
                    {
                        nativeLogDebug("connection failed: open failed");
                    }
                }
            }
            else
            {
                try
                {
                    usbSerialPort.open(usbConnection);
                    try
                    {
                        usbSerialPort.setParameters(baudRate, 8, 1, UsbSerialPort.PARITY_NONE);
                    }
                    catch (UnsupportedOperationException ex)
                    {
                        status("unsupported setParameters");
                    }
                    if(withIoManager)
                    {
                        usbIoManager = new SerialInputOutputManager(usbSerialPort, this);
                        usbIoManager.start();
                    }
                    connected = true;
                    controlLines.start();
                } catch (Exception e) {
                    status("connection failed: " + e.getMessage());
                    disconnect();
                }
            }
        }
    }

    private static void _handleUsbDetached(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(device != null)
        {
            final int deviceId = device.getDeviceId();
            if(_findDriver(deviceId) != null)
            {

            }
        }
    }

    private static UsbSerialDriver _findDriver(int deviceId)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbDrivers)
        {
            if (driver.getDevice().getDeviceId() == deviceId)
            {
                result = driver;
                break;
            }
        }

        return result;
    }

    private static UsbSerialDriver _findDriver(String deviceName)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbDrivers)
        {
            if (driver.getDevice().getDeviceName().equals(deviceName))
            {
                result = driver;
                break;
            }
        }

        return result;
    }

    private static UsbSerialDriver _findDriver(UsbDevice usbDevice)
    {
        return _usbProbeTable.findDriver(usbDevice);
    }

    private static SerialInputOutputManager _findUsbIoManager(int deviceId)
    {
        SerialInputOutputManager result = null;

        for (int i = 0; i < _usbDrivers.size(); i++)
        {
            if (_usbDrivers.get(i).getDevice().getDeviceId() == deviceId)
            {
                result = _usbIoManagers.get(i);
                break;
            }
        }

        return result;
    }

    private static SerialInputOutputManager _findUsbIoManager(String deviceName)
    {
        SerialInputOutputManager result = null;

        for (int i = 0; i < _usbDrivers.size(); i++)
        {
            if (_usbDrivers.get(i).getDevice().getDeviceName().equals(deviceName))
            {
                result = _usbIoManagers.get(i);
                break;
            }
        }

        return result;
    }

    private static void _updateCurrentDrivers()
    {
        List<UsbSerialDriver> drivers = _usbProber.findAllDrivers(_usbManager);

        for (int i = _usbDrivers.size() - 1; i >= 0; i--)
        {
            boolean found = false;
            for (UsbSerialDriver driver: drivers)
            {
                if (_usbDrivers.get(i).getDevice().getDeviceId() == driver.getDevice().getDeviceId())
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                _usbDrivers.remove(i);
            }
        }

        for (UsbSerialDriver newDriver: drivers)
        {
            boolean found = false;
            UsbDevice device = newDriver.getDevice();
            for (UsbSerialDriver driver: _usbDrivers)
            {
                if (device.getDeviceId() == driver.getDevice().getDeviceId())
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                _usbDrivers.add(newDriver);
                _createUsbIoManager(newDriver);
                nativeLogDebug("Adding new driver " + device.getDeviceName());

                if (!_usbManager.hasPermission(device))
                {
                    nativeLogDebug("Requesting permission to use device " + device.getDeviceName());
                    _usbManager.requestPermission(device, _usbPermissionIntent);
                    _usbPermission = UsbPermission.Requested;
                }
            }
        }
    }

    private static void _createUsbIoManager(UsbSerialDriver driver)
    {
        final int deviceId = driver.getDevice().getDeviceId();

        SerialInputOutputManager.Listener usbListener = new UsbIoManager.Listener()
        {
            @Override
            public void onRunError(Exception ex)
            {
                Log.e(TAG, "onRunError Exception: " + ex.getMessage());
            }

            @Override
            public void onNewData(final byte[] data)
            {
                Log.i(TAG, "onNewData");
                nativeDeviceNewData(deviceId, data);
            }
        };

        UsbSerialPort port = driver.getPorts().get(0);
        SerialInputOutputManager manager = new SerialInputOutputManager(port, usbListener);
        _usbIoManagers.add(manager);
    }

    private static void _startUsbIoManager(int deviceId)
    {
        SerialInputOutputManager manager = _findUsbIoManager(deviceId);

        if(manager != null)
        {
            manager.start();
        }
    }

    private static void _stopUsbIoManager(int deviceId)
    {
        SerialInputOutputManager manager = _findUsbIoManager(deviceId);

        if(manager != null)
        {
            manager.stop();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // USB Public Interface
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    public static String[] availableDevicesInfo()
    {
        _updateCurrentDrivers();

        List<String> deviceInfoList = new ArrayList<String>();

        for (UsbSerialDriver driver: _usbDrivers)
        {
            final UsbDevice device = driver.getDevice();
            String deviceInfo = device.getDeviceName() + ":";

            if (driver instanceof CdcAcmSerialDriver)
            {
                deviceInfo += "CdcAcm:";
            }
            else if (driver instanceof Ch34xSerialDriver)
            {
                deviceInfo += "Ch34x:";
            }
            else if (driver instanceof ChromeCcdSerialDriver)
            {
                deviceInfo += "ChromeCcd:";
            }
            else if (driver instanceof Cp21xxSerialDriver)
            {
                deviceInfo += "Cp21xx:";
            }
            else if (driver instanceof FtdiSerialDriver)
            {
                deviceInfo += "FTDI:";
            }
            else if (driver instanceof GsmModemSerialDriver)
            {
                deviceInfo += "GsmModem:";
            }
            else if (driver instanceof ProlificSerialDriver)
            {
                deviceInfo += "Prolific:";
            }
            else
            {
                deviceInfo += "Unknown:";
            }

            deviceInfo += Integer.toString(device.getProductId()) + ":";
            deviceInfo += Integer.toString(device.getVendorId()) + ":";

            deviceInfoList.add(deviceInfo);
        }

        String[] rgDeviceInfo = new String[deviceInfoList.size()];
        for (int i = 0; i < deviceInfoList.size(); i++)
        {
            rgDeviceInfo[i] = deviceInfoList.get(i);
        }

        return rgDeviceInfo;
    }

    public static int open(String deviceName)
    {
        int deviceId = BAD_DEVICE_ID;

        UsbSerialDriver driver = _findDriver(deviceName);

        if(driver != null)
        {
            UsbDevice device = driver.getDevice();
            if(device != null)
            {
                try
                {
                    driver.open(_usbManager.openDevice(device));
                }
                catch(IOException ex)
                {
                    nativeLogWarning("open exception: " + ex.getMessage());
                }

                if(driver.isOpen())
                {
                    deviceId = device.getDeviceId();
                    _startUsbIoManager(deviceId);
                }
            }
        }

        return deviceId;
    }

    public static void close(int deviceId)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.close();
            }
            catch(IOException ex)
            {
                nativeLogWarning("close exception: " + ex.getMessage());
            }

            if(!driver.isOpen())
            {
                _stopUsbIoManager(deviceId);
            }
        }
    }

    public static boolean isDeviceOpen(String name)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(name);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }

    public static boolean isDeviceOpen(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }

    public static int read(int deviceId, byte[] data, int timeoutMs)
    {
        int result = 0;

        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                result = driver.read(data, timeoutMs);
            }
            catch(IOException ex)
            {
                nativeLogWarning("read exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void write(int deviceId, byte[] data, int timeoutMs)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.write(data, timeoutMs);
            }
            catch(IOException ex)
            {
                nativeLogWarning("write exception: " + ex.getMessage());
            }
        }
    }

    public static void writeAsync(int deviceId, byte[] data, int timeoutMs)
    {
        SerialInputOutputManager manager = _findUsbIoManager(deviceId);

        if((manager != null) && (timeoutMs > 0))
        {
            manager.setWriteTimeout(timeoutMs);
            manager.writeAsync(data);
        }
    }

    public static void setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setParameters(baudRateA, dataBitsA, stopBitsA, parityA);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setParameters exception: " + ex.getMessage());
            }
        }
    }

    public static boolean getCarrierDetect(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCD();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getCarrierDetect exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getClearToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCTS();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getClearToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getDataSetReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDSR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getDataSetReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getDataTerminalReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getDataTerminalReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void setDataTerminalReady(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setDTR(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setDataTerminalReady exception: " + ex.getMessage());
            }
        }
    }

    public static boolean getRingIndicator(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getRingIndicator exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getRequestToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getRTS();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getRequestToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void setRequestToSend(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setRTS(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setRequestToSend exception: " + ex.getMessage());
            }
        }
    }

    public static void flush(int deviceId, boolean output, boolean input)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.purgeHwBuffers(output, input);
            }
            catch(IOException ex)
            {
                nativeLogWarning("flush exception: " + ex.getMessage());
            }
        }
    }

    public static void setBreak(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.setBreak(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setBreak exception: " + ex.getMessage());
            }
        }
    }
}
