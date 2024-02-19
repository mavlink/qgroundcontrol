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
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
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

    /* Multicasting */
    private static WifiManager.MulticastLock            _wifiMulticastLock;

    /* Screen */
    private static PowerManager.WakeLock                _wakeLock;

    /* USB */
    private static final int                            BAD_DEVICE_ID = 0;
    private static final String                         ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    private static ExecutorService                      _usbExecutor;
    private static UsbManager                           _usbManager;
    private static List<UsbSerialDriver>                _usbdrivers;
    private static List<UsbSerialPort>                  _usbPorts;
    private static List<SerialInputOutputManager>       _usbIoManagers;
    private static List<UsbDeviceConnection>            _usbConnections;
    private static HashMap<Integer, Long>               _usbIds;
    private static PendingIntent                        _usbPermissionIntent;
    private static SerialInputOutputManager.Listener    _usbListener;
    private static BroadcastReceiver                    _usbReceiver;
    private static UsbSerialProber                      _usbProber;

    // Native C++ functions which connect back to QSerialPort code
    private static native void nativeDeviceHasDisconnected(long userData);
    private static native void nativeDeviceException(long userData, String messageA);
    private static native void nativeDeviceNewData(long userData, byte[] dataA);
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

        PowerManager pm = (PowerManager) _instance.getSystemService(Context.POWER_SERVICE);
        _wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "QGroundControl");
        if(_wakeLock != null)
        {
            _wakeLock.acquire();
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
            Log.d(TAG, "Multicast lock: " + _wifiMulticastLock.toString());
        }
        else
        {
            Log.i(TAG, "MulticastLock not acquired!!!");
        }

        createUsbHandler();

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

    private static void createUsbHandler()
    {
        _usbdrivers = new ArrayList<UsbSerialDriver>();
        _usbIds = new HashMap<Integer, Long>();
        _usbIoManagers = new HashMap<Integer, UsbIoManager>();

        _usbExecutor = Executors.newSingleThreadExecutor();

        /* TODO: Create custom prober */
        /* ProbeTable customTable = new ProbeTable();
        customTable.addProduct(0x1234, 0x0001, FtdiSerialDriver.class);
        customTable.addProduct(0x1234, 0x0002, FtdiSerialDriver.class);

        _usbProber = new UsbSerialProber(customTable); */
        _usbProber = UsbSerialProber.getDefaultProber();

        _usbListener = new UsbIoManager.Listener()
        {
            @Override
            public void onRunError(Exception eA, long userData)
            {
                Log.e(TAG, "onRunError Exception");
                nativeDeviceException(userData, eA.getMessage());
            }

            @Override
            public void onNewData(final byte[] dataA, long userData)
            {
                nativeDeviceNewData(userData, dataA);
            }
        };

        _usbReceiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                String action = intent.getAction();
                Log.i(TAG, "BroadcastReceiver USB action " + action);

                if (ACTION_USB_PERMISSION.equals(action)) {
                    synchronized (_instance) {
                        UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                        if (device != null) {
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
                } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                    UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null) {
                        if (_usbIds.containsKey(device.getDeviceId())) {
                            nativeDeviceHasDisconnected(_usbIds.get(device.getDeviceId()));
                        }
                    }
                }

                // FIXME-QT6: Not ye converted to Qt6
                //try {
                //    nativeUpdateAvailableJoysticks();
                //} catch(Exception e) {
                //    Log.e(TAG, "Exception nativeUpdateAvailableJoysticks()");
                //}
            }
        };

        _usbManager = (UsbManager)_instance.getSystemService(Context.USB_SERVICE);

        // Register for USB Detach and USB Permission intent
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        _instance.registerReceiver(_instance._usbReceiver, filter);

        // Create intent for usb permission request
        int intentFlags = 0;
        if (android.os.Build.VERSION.SDK_INT >= 23) {
            intentFlags = PendingIntent.FLAG_IMMUTABLE;
        }
        _usbPermissionIntent = PendingIntent.getBroadcast(_instance, 0, new Intent(ACTION_USB_PERMISSION), intentFlags);
    }

    private static UsbSerialDriver _findDriverByDeviceId(int deviceId)
    {
        for (UsbSerialDriver driver: _usbdrivers)
        {
            if (driver.getDevice().getDeviceId() == deviceId)
            {
                return driver;
            }
        }
        return null;
    }

    private static UsbSerialDriver _findDriverByDeviceName(String deviceName)
    {
        for (UsbSerialDriver driver: _usbdrivers)
        {
            if (driver.getDevice().getDeviceName().equals(deviceName))
            {
                return driver;
            }
        }
        return null;
    }

    /// Incrementally updates the list of drivers connected to the device
    private static void updateCurrentDrivers()
    {
        List<UsbSerialDriver> currentDrivers = _usbProber.findAllDrivers(_usbManager);

        // Remove stale drivers
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

        // Add new drivers
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

                // Request permission if needed
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

    /// Returns array of device info for each unopened device.
    /// @return Device info format DeviceName:Company:ProductId:VendorId
    public static String[] availableDevicesInfo()
    {
        updateCurrentDrivers();

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

    /// Open the specified device
    ///     @param userData Data to associate with device and pass back through to native calls.
    /// @return Device id
    public static int open(Context parentContext, String deviceName, long userData)
    {
        int deviceId = BAD_DEVICE_ID;

        UsbSerialDriver driver = _findDriverByDeviceName(deviceName);
        if (driver == null) {
            qgcLogWarning("Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        if (driver.permissionStatus() != UsbSerialDriver.permissionStatusSuccess) {
            qgcLogWarning("Attempt to open device with incorrect permission status " + deviceName + " " + driver.permissionStatus());
            return BAD_DEVICE_ID;
        }

        UsbDevice device = driver.getDevice();
        deviceId = device.getDeviceId();

        try {
            driver.setConnection(_usbManager.openDevice(device));
            driver.open();
            driver.setPermissionStatus(UsbSerialDriver.permissionStatusOpen);

            _usbIds.put(deviceId, userData);

            UsbIoManager ioManager = new UsbIoManager(driver, _usbListener, userData);
            _usbIoManagers.put(deviceId, ioManager);
            _usbExecutor.submit(ioManager);

            qgcLogDebug("Port open successful");
        } catch(IOException exA) {
            driver.setPermissionStatus(UsbSerialDriver.permissionStatusRequestRequired);
            _usbIds.remove(deviceId);

            if(_usbIoManagers.get(deviceId) != null) {
                _usbIoManagers.get(deviceId).stop();
                _usbIoManagers.remove(deviceId);
            }
            qgcLogWarning("Port open exception: " + exA.getMessage());
            return BAD_DEVICE_ID;
        }

        return deviceId;
    }

    public static void startIoManager(int idA)
    {
        if (_usbIoManagers.get(idA) != null)
            return;

        UsbSerialDriver driverL = _findDriverByDeviceId(idA);

        if (driverL == null)
            return;

        UsbIoManager managerL = new UsbIoManager(driverL, _usbListener, _usbIds.get(idA));
        _usbIoManagers.put(idA, managerL);
        _usbExecutor.submit(managerL);
    }

    public static void stopIoManager(int idA)
    {
        if(_usbIoManagers.get(idA) == null)
            return;

        _usbIoManagers.get(idA).stop();
        _usbIoManagers.remove(idA);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Sets the parameters on an open port.
    //
    //  Args:   idA - ID number from the open command
    //          baudRateA - Decimal value of the baud rate.  I.E. 9600, 57600, 115200, etc.
    //          dataBitsA - number of data bits.  Valid numbers are 5, 6, 7, 8
    //          stopBitsA - number of stop bits.  Valid numbers are 1, 2
    //          parityA - No Parity=0, Odd Parity=1, Even Parity=2
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setParameters(int idA, int baudRateA, int dataBitsA, int stopBitsA, int parityA)
    {
        UsbSerialDriver driverL = _findDriverByDeviceId(idA);

        if (driverL == null)
            return false;

        try
        {
            driverL.setParameters(baudRateA, dataBitsA, stopBitsA, parityA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Close the device.
    //
    //  Args:  idA - ID number from the open command
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean close(int idA)
    {
        UsbSerialDriver driverL = _findDriverByDeviceId(idA);

        if (driverL == null)
            return false;

        try
        {
            stopIoManager(idA);
            _usbIds.remove(idA);
            driverL.setPermissionStatus(UsbSerialDriver.permissionStatusRequestRequired);
            driverL.close();

            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Write data to the device.
    //
    //  Args:   idA - ID number from the open command
    //          sourceA - byte array of data to write
    //          timeoutMsecA - amount of time in milliseconds to wait for the write to occur
    //
    //  Returns:  number of bytes written
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    public static int write(int idA, byte[] sourceA, int timeoutMSecA)
    {
        UsbSerialDriver driverL = _findDriverByDeviceId(idA);

        if (driverL == null)
            return 0;

        try
        {
            return driverL.write(sourceA, timeoutMSecA);
        }
        catch(IOException eA)
        {
            return 0;
        }
        /*
        UsbIoManager managerL = _usbIoManagers.get(idA);

        if(managerL != null)
        {
            managerL.writeAsync(sourceA);
            return sourceA.length;
        }
        else
            return 0;
        */
    }

    public static boolean isDeviceNameValid(String nameA)
    {
        for (UsbSerialDriver driver: _usbdrivers) {
            if (driver.getDevice().getDeviceName() == nameA)
                return true;
        }

        return false;
    }

    public static boolean isDeviceNameOpen(String nameA)
    {
        for (UsbSerialDriver driverL: _usbdrivers) {
            if (nameA.equals(driverL.getDevice().getDeviceName()) && driverL.permissionStatus() == UsbSerialDriver.permissionStatusOpen) {
                return true;
            }
        }

        return false;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the Data Terminal Ready flag on the device
    //
    //  Args:   idA - ID number from the open command
    //          onA - on=T, off=F
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setDataTerminalReady(int idA, boolean onA)
    {
        try
        {
            UsbSerialDriver driverL = _findDriverByDeviceId(idA);

            if (driverL == null)
                return false;

            driverL.setDTR(onA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the Request to Send flag
    //
    //  Args:   idA - ID number from the open command
    //          onA - on=T, off=F
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setRequestToSend(int idA, boolean onA)
    {
        try
        {
            UsbSerialDriver driverL = _findDriverByDeviceId(idA);

            if (driverL == null)
                return false;

            driverL.setRTS(onA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Purge the hardware buffers based on the input and output flags
    //
    //  Args:   idA - ID number from the open command
    //          inputA - input buffer purge.  purge=T
    //          outputA - output buffer purge.  purge=T
    //
    //  Returns:  T/F Success/Failure
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean purgeBuffers(int idA, boolean inputA, boolean outputA)
    {
        try
        {
            UsbSerialDriver driverL = _findDriverByDeviceId(idA);

            if (driverL == null)
                return false;

            return driverL.purgeHwBuffers(inputA, outputA);
        }
        catch(IOException eA)
        {
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Get the native device handle (file descriptor)
    //
    //  Args:   idA - ID number from the open command
    //
    //  Returns:  device handle
    //
    ///////////////////////////////////////////////////////////////////////////////////////////
    public static int getDeviceHandle(int idA)
    {
        UsbSerialDriver driverL = _findDriverByDeviceId(idA);

        if (driverL == null)
            return -1;

        UsbDeviceConnection connectL = driverL.getDeviceConnection();
        if (connectL == null)
            return -1;
        else
            return connectL.getFileDescriptor();
    }

    public static String getSDCardPath() {
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
}

