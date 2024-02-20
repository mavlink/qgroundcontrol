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
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
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
import android.widget.Toast;

import com.hoho.android.usbserial.driver.*;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity
{
    /* General */
    private static QGCActivity                          _instance = null;
    private static final String                         TAG = "QGC_QGCActivity";
    private static BroadcastReceiver                    _receiver;

    /* Multicasting */
    private static WifiManager.MulticastLock            _wifiMulticastLock;

    /* Screen */
    private static PowerManager.WakeLock                _wakeLock;

    /* USB */
    private static final int                            BAD_DEVICE_ID = 0;
    private static final String                         ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    private static UsbManager                           _usbManager;
    private static List<UsbSerialDriver>                _usbdrivers;
    private static List<SerialInputOutputManager>       _usbIoManagers;
    private static UsbSerialProber                      _usbProber;
    private static PendingIntent                        _usbPermissionIntent;

    // Native C++ functions which connect back to QSerialPort code
    private static native void nativeDeviceHasDisconnected(int deviceId);
    private static native void nativeDeviceHasConnected(int deviceId);
    private static native void nativeDeviceException(int deviceId, String message);
    private static native void nativeDeviceNewData(int deviceId, byte[] data);
    private static native void nativeUpdateAvailableJoysticks();

    // Native C++ functions called to log output
    public static native void qgcLogDebug(String message);
    public static native void qgcLogWarning(String message);

    public native void nativeInit();

    // QGCActivity singleton
    public QGCActivity()
    {
        _instance = this;
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        _receiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                String action = intent.getAction();
                Log.i(TAG, "BroadcastReceiver action " + action);

                if(action.equals(ACTION_USB_PERMISSION))
                {
                    synchronized(_instance)
                    {
                        _handleUsbPermissions(intent);
                    }
                }
                else if(action.equals(UsbManager.ACTION_USB_DEVICE_DETACHED))
                {
                    _handleUsbDetached(intent);
                }
                else if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED))
                {
                    _handleUsbAttached(intent);
                }
                else if(action.equals(BluetoothDevice.ACTION_ACL_CONNECTED))
                {
                    /* _handleBluetoothConnected(intent); */
                }
                else if(action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED))
                {
                    /* _handleBluetoothDisconnected(intent); */
                }

                /* nativeUpdateAvailableJoysticks(); */
            }
        };

        IntentFilter bluetoothFilter = new IntentFilter();
        bluetoothFilter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        bluetoothFilter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        _instance.registerReceiver(_receiver, bluetoothFilter);

        PowerManager pm = (PowerManager) _instance.getSystemService(Context.POWER_SERVICE);
        _wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "QGroundControl");
        if(_wakeLock != null)
        {
            _wakeLock.acquire();
            Log.d(TAG, "SCREEN_BRIGHT_WAKE_LOCK: " + _wakeLock.toString());
        }
        else
        {
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!!!");
        }
        _instance.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Workaround for QTBUG-73138
        WifiManager wifi = (WifiManager) _instance.getSystemService(Context.WIFI_SERVICE);
        _wifiMulticastLock = wifi.createMulticastLock("QGroundControl");
        if(_wakeLock != null)
        {
            _wifiMulticastLock.setReferenceCounted(true);
            _wifiMulticastLock.acquire();
            Log.d(TAG, "WifiMulticastLock: " + _wifiMulticastLock.toString());
        }
        else
        {
            Log.i(TAG, "WifiMulticastLock not acquired!!!");
        }

        _createUsbHandler();

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
                Log.d(TAG, "Multicast lock released.");
            }
            if(_wakeLock != null)
            {
                _wakeLock.release();
            }
        }
        catch(Exception e)
        {
           Log.e(TAG, "Exception onDestroy()");
        }

        super.onDestroy();
    }

    public void onInit(int status)
    {

    }

    public static String getSDCardPath()
    {
        StorageManager storageManager = (StorageManager)_instance.getSystemService(Activity.STORAGE_SERVICE);
        List<StorageVolume> volumes = storageManager.getStorageVolumes();
        Method mMethodGetPath;
        String path = "";
        for (StorageVolume vol : volumes) {
            try {
                mMethodGetPath = vol.getClass().getMethod("getPath");
            } catch (NoSuchMethodException e) {
                e.printStackTrace();
                continue;
            }
            try {
                path = (String) mMethodGetPath.invoke(vol);
            } catch (Exception e) {
                e.printStackTrace();
                continue;
            }

            if (vol.isRemovable() == true) {
                Log.i(TAG, "removable sd card mounted " + path);
                return path;
            } else {
                Log.i(TAG, "storage mounted " + path);
            }
        }
        return "";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // USB Private Helpers
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    private static void _createUsbHandler()
    {
        _usbdrivers = new ArrayList<UsbSerialDriver>();
        _usbPortPtrs = new HashMap<Integer, Long>();
        _usbIoManagers = new HashMap<Integer, UsbIoManager>();

        /* TODO: Create custom prober */
        /* ProbeTable customTable = new ProbeTable();
        customTable.addProduct(0x1234, 0x0001, FtdiSerialDriver.class);
        customTable.addProduct(0x1234, 0x0002, FtdiSerialDriver.class);
        _usbProber = new UsbSerialProber(customTable); */
        _usbProber = UsbSerialProber.getDefaultProber();
        _usbManager = (UsbManager)_instance.getSystemService(Context.USB_SERVICE);

        // Register for USB Detach and USB Permission intent
        IntentFilter usbFilter = new IntentFilter();
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        usbFilter.addAction(ACTION_USB_PERMISSION);
        _instance.registerReceiver(_receiver, usbFilter);
        _usbPermissionIntent = PendingIntent.getBroadcast(_instance, 0, new Intent(ACTION_USB_PERMISSION), 0);
    }

    private static void _handleUsbPermissions(Intent intent)
    {
        UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            UsbSerialDriver driver = _findDriverByDeviceId(device.getDeviceId());
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                qgcLogDebug("Permission granted to " + device.getDeviceName());
                driver.setPermissionStatus(UsbSerialDriver.permissionStatusSuccess);
            } else {
                qgcLogDebug("Permission denied for " + device.getDeviceName());
                driver.setPermissionStatus(UsbSerialDriver.permissionStatusDenied);
            }
        }
    }

    private static void _handleUsbAttached(Intent intent)
    {
        UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            if (_usbPortPtrs.containsKey(device.getDeviceId()))
            {
                nativeDeviceHasConnected(_usbPortPtrs.get(device.getDeviceId()));
            }
        }
    }

    private static void _handleUsbDetached(Intent intent)
    {
        UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(device != null)
        {
            if(_usbPortPtrs.containsKey(device.getDeviceId()))
            {
                nativeDeviceHasDisconnected(_usbPortPtrs.get(device.getDeviceId()));
            }
        }
    }

    private static UsbSerialDriver _findDriverByDeviceId(int deviceId)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbdrivers)
        {
            if (driver.getDevice().getDeviceId() == deviceId)
            {
                result = driver;
                break;
            }
        }

        return result;
    }

    private static UsbSerialDriver _findDriverByDeviceName(String deviceName)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbdrivers)
        {
            if (driver.getDevice().getDeviceName().equals(deviceName))
            {
                result = driver;
                break;
            }
        }
        return result;
    }

    private static void _updateCurrentDrivers()
    {
        List<UsbSerialDriver> currentDrivers = _usbProber.findAllDrivers(_usbManager);

        for (int i=_usbdrivers.size()-1; i>=0; i--) {
            boolean found = false;
            for (UsbSerialDriver currentDriver: currentDrivers) {
                if (_usbdrivers.get(i).getDevice().getDeviceId() == currentDriver.getDevice().getDeviceId()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                qgcLogDebug("Remove stale driver " + _usbdrivers.get(i).getDevice().getDeviceName());
                _usbdrivers.remove(i);
            }
        }

        for (int i=0; i<currentDrivers.size(); i++) {
            boolean found = false;
            for (int j=0; j<_usbdrivers.size(); j++) {
                if (currentDrivers.get(i).getDevice().getDeviceId() == _usbdrivers.get(j).getDevice().getDeviceId()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                UsbSerialDriver newDriver =     currentDrivers.get(i);
                UsbDevice       device =        newDriver.getDevice();
                String          deviceName =    device.getDeviceName();

                _usbdrivers.add(newDriver);
                qgcLogDebug("Adding new driver " + deviceName);

                if (_usbManager.hasPermission(device)) {
                    qgcLogDebug("Already have permission to use device " + deviceName);
                    newDriver.setPermissionStatus(UsbSerialDriver.permissionStatusSuccess);
                } else {
                    qgcLogDebug("Requesting permission to use device " + deviceName);
                    newDriver.setPermissionStatus(UsbSerialDriver.permissionStatusRequested);
                    _usbManager.requestPermission(device, _usbPermissionIntent);
                }
            }
        }
    }

    private static boolean _startUsbIoManager(int deviceId)
    {
        boolean result = false;

        UsbSerialPort usbPort = _findDriverByDeviceId(deviceId);

        if (usbPort == null)
        {
            return false;
        }

        if (_usbIoManagers.get(portId) == null)
        {
            return false;
        }

        SerialInputOutputManager.Listener usbListener = new UsbIoManager.Listener()
        {
            @Override
            public void onRunError(Exception eA)
            {
                Log.e(TAG, "onRunError Exception");
                nativeDeviceException(eA.getMessage());
            }

            @Override
            public void onNewData(final byte[] dataA)
            {
                nativeDeviceNewData(dataA);
            }
        };

        SerialInputOutputManager manager = new SerialInputOutputManager(usbPort, usbListener);
        _usbIoManagers.put(portId, manager);

        return true;
    }

    private static void _stopUsbIoManager(int deviceId)
    {
        SerialInputOutputManager manager = _usbIoManagers.get(deviceId);

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

        if (_usbdrivers.size() <= 0)
        {
            return null;
        }

        List<String> deviceInfoList = new ArrayList<String>();

        for (int i=0; i<_usbdrivers.size(); i++) {
            String          deviceInfo;
            UsbSerialDriver driver = _usbdrivers.get(i);

            if (driver.permissionStatus() != UsbSerialDriver.permissionStatusSuccess) {
                continue;
            }

            UsbDevice device = driver.getDevice();

            deviceInfo = device.getDeviceName() + ":";

            if (driver instanceof FtdiSerialDriver) {
                deviceInfo = deviceInfo + "FTDI:";
            } else if (driver instanceof CdcAcmSerialDriver) {
                deviceInfo = deviceInfo + "Cdc Acm:";
            } else if (driver instanceof Cp2102SerialDriver) {
                deviceInfo = deviceInfo + "Cp2102:";
            } else if (driver instanceof ProlificSerialDriver) {
                deviceInfo = deviceInfo + "Prolific:";
            } else {
                deviceInfo = deviceInfo + "Unknown:";
            }

            deviceInfo = deviceInfo + Integer.toString(device.getProductId()) + ":";
            deviceInfo = deviceInfo + Integer.toString(device.getVendorId()) + ":";

            deviceInfoList.add(deviceInfo);
        }

        String[] rgDeviceInfo = new String[deviceInfoList.size()];
        for (int i=0; i<deviceInfoList.size(); i++) {
            rgDeviceInfo[i] = deviceInfoList.get(i);
        }

        return rgDeviceInfo;
    }

    public static int open(Context parentContext, String deviceName)
    {
        int deviceId = BAD_DEVICE_ID;

        UsbSerialDriver driver = _findDriverByDeviceName(deviceName);
        if (driver == null) {
            qgcLogWarning("Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        UsbDevice device = driver.getDevice();
        deviceId = device.getDeviceId();

        try {
            driver.setConnection(_usbManager.openDevice(device));
            driver.open();

            _usbPortPtrs.put(deviceId);

            startPort(portId);

            qgcLogDebug("Port open successful");
        } catch(IOException ex) {
            _usbPortPtrs.remove(deviceId);

            stopPort(portId);

            qgcLogWarning("open exception: " + ex.getMessage());
            deviceId = BAD_DEVICE_ID;
        }

        return deviceId;
    }

    public static void close(int deviceId)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            try
            {
                _stopUsbIoManager(deviceId);
                driver.close();
            }
            catch(IOException ex)
            {
                qgcLogWarning("close exception: " + ex.getMessage());
            }
        }
    }

    public static int read(int deviceId, byte[] data, int timeoutMS)
    {
        int result = 0;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            try
            {
                result = driver.read(data, timeoutMS);
            }
            catch(IOException ex)
            {
                qgcLogWarning("read exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void write(int deviceId, byte[] data, int timeoutMS)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            try
            {
                driver.write(data, timeoutMS);
            }
            catch(IOException ex)
            {
                qgcLogWarning("write exception: " + ex.getMessage());
            }
        }
    }

    public static void writeAsync(int deviceId, byte[] data, int timeoutMS)
    {
        SerialInputOutputManager manager = _usbIoManagers.get(deviceId);

        if((manager != null) && (timeoutMS > 0))
        {
            manager.setWriteTimeout(timeoutMS);
            manager.writeAsync(data);
        }
    }

    public static void setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setParameters(baudRateA, dataBitsA, stopBitsA, parityA);
            }
            catch(IOException ex)
            {
                qgcLogWarning("setParameters exception: " + ex.getMessage());
            }
        }
    }

    public static boolean getCarrierDetect(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCD();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getCarrierDetect exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getClearToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCTS();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getClearToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getDataSetReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDSR();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getDataSetReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getDataTerminalReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getDataTerminalReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void setDataTerminalReady(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setDTR(on);
            }
            catch(IOException ex)
            {
                qgcLogWarning("setDataTerminalReady exception: " + ex.getMessage());
            }
        }
    }

    public static boolean getRingIndicator(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getRingIndicator exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static boolean getRequestToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getRTS();
            }
            catch(IOException ex)
            {
                qgcLogWarning("getRequestToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public static void setRequestToSend(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setRTS(on);
            }
            catch(IOException ex)
            {
                qgcLogWarning("setRequestToSend exception: " + ex.getMessage());
            }
        }
    }

    public static void flush(int deviceId, boolean output, boolean input)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            try
            {
                driver.purgeHwBuffers(output, input);
            }
            catch(IOException ex)
            {
                qgcLogWarning("flush exception: " + ex.getMessage());
            }
        }
    }

    public static void setBreak(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            try
            {
                driver.setBreak(on);
            }
            catch(IOException ex)
            {
                qgcLogWarning("setBreak exception: " + ex.getMessage());
            }
        }
    }

    public static boolean isDeviceOpen(String name)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceName(name);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }

    public static boolean isDeviceOpen(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriverByDeviceId(deviceId);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }
}
