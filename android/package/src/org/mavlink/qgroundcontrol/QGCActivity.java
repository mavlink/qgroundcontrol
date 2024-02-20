package org.mavlink.qgroundcontrol;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.WindowManager;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity
{
    /* General */
    private static final String TAG = getSimpleName();

    /* Multicasting */
    private static WifiManager.MulticastLock _wifiMulticastLock;

    /* USB */
    private static UsbSerialInterface _usbSerialInterface;

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

        BroadcastReceiver receiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                String action = intent.getAction();
                Log.i(TAG, "BroadcastReceiver action " + action);

                if(action.equals(BluetoothDevice.ACTION_ACL_CONNECTED))
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
        registerReceiver(receiver, bluetoothFilter);

        _usbSerialInterface = new UsbSerialInterface(this);

        nativeInit();
    }

    @Override
    protected void onDestroy()
    {
        if (_wifiMulticastLock != null)
        {
            try
            {
                _wifiMulticastLock.release();
                Log.d(TAG, "WifiMulticastLock released.");
            }
            catch(Exception ex)
            {
               Log.e(TAG, "Exception onDestroy WifiMulticastLock: " + ex.getMessage());
            }
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

            try
            {
                mMethodGetPath = vol.getClass().getMethod("getPath");
            }
            catch (NoSuchMethodException ex)
            {
                Log.e(TAG, "Exception getSDCardPath mMethodGetPath: " + ex.getMessage());
                e.printStackTrace();
                continue;
            }

            String path;

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
}
