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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.Timer;
import java.util.TimerTask;
import java.io.IOException;
import java.lang.reflect.Method;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.widget.Toast;
import android.util.Log;
import android.os.PowerManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.app.PendingIntent;
import android.view.WindowManager;
import android.os.Bundle;
import android.bluetooth.BluetoothDevice;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import org.qtproject.qt.android.bindings.QtActivity;
import org.qtproject.qt.android.bindings.QtApplication;

public class QGCActivity extends QtActivity
{
    public  static int                                  BAD_DEVICE_ID = 0;
    private static QGCActivity                          _instance = null;
    private static final String                         TAG = "QGC_QGCActivity";
    private static PowerManager.WakeLock                _wakeLock;
    private static final String                         ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    private static PendingIntent                        _usbPermissionIntent = null;
    private static WifiManager.MulticastLock            _wifiMulticastLock;
    
    public static Context m_context;

    private final static ExecutorService m_Executor = Executors.newSingleThreadExecutor();

    private final static BroadcastReceiver _usbReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                Log.i(TAG, "BroadcastReceiver USB action " + action);

                try {
                    // FIXME-QT6 - This is not working
                    //nativeUpdateAvailableJoysticks();
                } catch(Exception e) {
                    Log.e(TAG, "Exception nativeUpdateAvailableJoysticks()");
                }
            }
        };

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
        nativeInit();
        PowerManager pm = (PowerManager)_instance.getSystemService(Context.POWER_SERVICE);
        _wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "QGroundControl");
        if(_wakeLock != null) {
            _wakeLock.acquire();
        } else {
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!!!");
        }
        _instance.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Workaround for QTBUG-73138
        if (_wifiMulticastLock == null) {
            WifiManager wifi = (WifiManager) _instance.getSystemService(Context.WIFI_SERVICE);
            _wifiMulticastLock = wifi.createMulticastLock("QGroundControl");
            _wifiMulticastLock.setReferenceCounted(true);
        }
        _wifiMulticastLock.acquire();
        Log.d(TAG, "Multicast lock: " + _wifiMulticastLock.toString());
    }

    @Override
    protected void onDestroy()
    {
        try {
            if (_wifiMulticastLock != null) {
                _wifiMulticastLock.release();
                Log.d(TAG, "Multicast lock released.");
            }
            if(_wakeLock != null) {
                _wakeLock.release();
            }
        } catch(Exception e) {
           Log.e(TAG, "Exception onDestroy()");
        }
        super.onDestroy();
    }

    public void onInit(int status) {
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

