package com.yuneec.datapilot;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.WindowManager;

import java.util.List;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.Status;

import android.os.BatteryManager;

import org.qtproject.qt5.android.bindings.QtActivity;
import org.qtproject.qt5.android.bindings.QtApplication;

public class QGCActivity extends QtActivity
{

    public enum ReceiverMode {
        DISABLED,
        SCANNING,
        BINDING
    }

    private static QGCActivity m_instance;
    private static final String TAG = "DataPilot_JNI";
    private static PowerManager.WakeLock m_wl;
    private static WifiManager mainWifi;
    private static WifiReceiver receiverWifi;
    private static ReceiverMode receiverMode = ReceiverMode.DISABLED;
    private static String currentConnection;
    private static int currentWifiRssi = 0;
    private static float batteryLevel = 0.0f;
    private static boolean letItExit = false;
    private static String lastAttemptedSSID;

    // WiFi: https://stackoverflow.com/questions/36098871/how-to-search-and-connect-to-a-specific-wifi-network-in-android-programmatically/36099552#36099552

    private static native void nativeNewWifiItem(String ssid, int rssi);
    private static native void nativeNewWifiRSSI();
    private static native void nativeScanComplete();
    private static native void nativeAuthError();
    private static native void nativeWifiConnected();
    private static native void nativeWifiDisconnected();
    private static native void nativeBatteryUpdate();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Constructor.  Only used once to create the initial instance for the static functions.
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////
    public QGCActivity() {
        m_instance = this;
        Log.i(TAG, "Instance created");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize Power Management
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate()");
        super.onCreate(savedInstanceState);
        PowerManager pm = (PowerManager)m_instance.getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "DataPilot");
        if(m_wl != null) {
            m_wl.acquire();
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK acquired.");
        } else {
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!!!");
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        //-- Full Screen
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        //-- WiFi
        mainWifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        mainWifi.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "QGCScanner");
        receiverWifi = new WifiReceiver();
        if(mainWifi.isWifiEnabled() == false) {
            mainWifi.setWifiEnabled(true);
        }
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_BATTERY_CHANGED);
        filter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        filter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        filter.addAction(WifiManager.RSSI_CHANGED_ACTION);
        registerReceiver(receiverWifi, filter);
        findWifiConfig();
    }

    @Override
    protected void onDestroy() {
        try {
            unregisterReceiver(receiverWifi);
            if(m_wl != null) {
                m_wl.release();
                Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK released.");
            }
        } catch(Exception e) {
           Log.e(TAG, "Exception onDestroy()");
        }
        super.onDestroy();
    }

    @Override
    protected void onStop() {
        if(!letItExit) {
            Intent intent = new Intent();
            intent.setClassName("com.android.launcher", "com.android.launcher2.Launcher");
            startActivity(intent);
        }
        super.onStop();
    }

    public void onInit(int status) {
    }

    public static void keepScreenOn() {
    }

    public static void restoreScreenOn() {
    }

    //-------------------------------------------------------------------------
    public static void launchFactoryTest() {
        m_instance.launchApp("com.yuneec.flightcontrolmodetest");
    }

    //-------------------------------------------------------------------------
    public static void launchUpdater() {
        m_instance.launchApp("com.yuneec.updater");
    }

    //-------------------------------------------------------------------------
    public static void launchApp(String app) {
        Intent launchIntent = m_instance.getPackageManager().getLaunchIntentForPackage(app);
        if (launchIntent != null) {
            letItExit = true;
            m_instance.startActivity(launchIntent);
            m_instance.finish();
        } else {
            Log.i(TAG, app + " not found.");
        }
    }

    //-------------------------------------------------------------------------
    public static void restartApp() {
        letItExit = false;
        m_instance.finish();
    }

    //-------------------------------------------------------------------------
    public static boolean isFactoryAppInstalled() {
        return m_instance.isAppInstalled("com.yuneec.flightcontrolmodetest");
    }

    //-------------------------------------------------------------------------
    public static boolean isUpdaterAppInstalled() {
        return m_instance.isAppInstalled("com.yuneec.updater");
    }

    //-------------------------------------------------------------------------
    public static boolean isAppInstalled(String app) {
        try {
            PackageManager pm = m_instance.getPackageManager();
            pm.getPackageInfo(app, 0);
            return true;
        } catch (NameNotFoundException e) {
            return false;
        }
    }

    //-------------------------------------------------------------------------
    public static void updateImage() {
        //-- By the time this function is called, the caller has
        //   to make sure "/storage/sdcard1/update.zip" has been
        //   copied to "/mnt/sdcard/update.zip". No checks are
        //   done here.
        Log.d(TAG,"Update invoked");
        //-- "/mnt/sdcard" is a 0777 symlink to "/data/media/0"
        String file = "/data/media/0/update.zip";
        try {
            Intent i = new Intent();
            i.setClassName("com.ota.reboot", "com.ota.reboot.RebootActivity");
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            i.putExtra("file", file);
            m_instance.startActivity(i);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    //-------------------------------------------------------------------------
    public static void disconnectWifi() {
        mainWifi.disconnect();
    }

    //-------------------------------------------------------------------------
    public static void reconnectWifi() {
        mainWifi.reconnect();
    }

    //-------------------------------------------------------------------------
    public static int wifiRssi() {
        return currentWifiRssi;
    }

    //-------------------------------------------------------------------------
    public static float getBatteryLevel() {
        return batteryLevel;
    }

    //-------------------------------------------------------------------------
    public static void launchBrowser(String url) {
        Log.i(TAG, "launchBrowser(): " + url);
        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        if (launchIntent != null) {
            letItExit = true;
            m_instance.startActivity(launchIntent);
            m_instance.finish();
        } else {
            Log.e(TAG, "Error launching browser.");
        }
    }

    //-------------------------------------------------------------------------
    public static void findWifiConfig() {
        String currentCamera = "";
        List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
        if(list != null) {
            for( WifiConfiguration i : list ) {
                if(i.SSID != null) {
                    Log.i(TAG, "Found config: " + i.SSID + " | " + i.priority);
                    if(i.status == 0) {
                        currentConnection = i.SSID;
                    }
                    if(i.SSID.startsWith("CGOET") || i.SSID.startsWith("E10T") || i.SSID.startsWith("E90_") || i.SSID.startsWith("E50_")) {
                        i.priority = 1;
                        currentCamera = i.SSID;
                    } else {
                        i.priority = 10;
                    }
                }
            }
            if(currentConnection == "") {
                currentConnection = currentCamera;
            }
            mainWifi.saveConfiguration();
        }
    }

    //-------------------------------------------------------------------------
    public static void resetWifi() {
        Log.i(TAG, "resetWifi()");
        List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
        if(list != null) {
            //-- Disable everything
            mainWifi.disconnect();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null) {
                    mainWifi.disableNetwork(i.networkId);
                }
            }
        }
        currentConnection = "";
        mainWifi.saveConfiguration();
        currentWifiRssi = 0;
        nativeNewWifiRSSI();
        nativeWifiDisconnected();
    }

    //-------------------------------------------------------------------------
    public static void setWifiPassword(String ssid, String password) {
        Log.i(TAG, "setWifiPassword(): " + ssid + " => " + password);
        List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
        if(list != null) {
            mainWifi.disconnect();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                    Log.i(TAG, "Forget " + ssid);
                    mainWifi.removeNetwork(i.networkId);
                }
            }
        }
        mainWifi.saveConfiguration();
        //-- Disconnect
        resetWifi();
        //-- Set new configuration (but don't connect as camera needs to reboot)
        bindSSID(ssid, password, false);
    }

    //-------------------------------------------------------------------------
    public static void forgetSSID(String ssid) {
        Log.i(TAG, "forgetSSID(): " + ssid);
        List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
        if(list != null) {
            mainWifi.disconnect();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                    Log.i(TAG, "Forget " + ssid);
                    mainWifi.removeNetwork(i.networkId);
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    public static void bindSSID(String ssid, String passphrase, boolean attemptConnect) {
        Log.i(TAG, "Bind: " + ssid + " " + passphrase + " " + attemptConnect);
        try {
            if(attemptConnect) {
                lastAttemptedSSID = ssid;
            } else {
                lastAttemptedSSID = "";
            }
            List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
            //-- Disable everything else
            mainWifi.disconnect();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null && !i.SSID.equals("\"" + ssid + "\"")) {
                    i.priority = 10;
                    mainWifi.disableNetwork(i.networkId);
                }
            }
            mainWifi.saveConfiguration();
            receiverMode = ReceiverMode.BINDING;
            //-- If already set, set password an make sure it's enabled
            list = mainWifi.getConfiguredNetworks();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                    i.preSharedKey = "\"" + passphrase + "\"";
                    i.priority = 1;
                    mainWifi.updateNetwork(i);
                    mainWifi.enableNetwork(i.networkId, attemptConnect);
                    mainWifi.reconnect();
                    mainWifi.saveConfiguration();
                    return;
                }
            }
            //-- Add new configuration
            WifiConfiguration conf = new WifiConfiguration();
            conf.SSID = "\"" + ssid + "\"";
            conf.preSharedKey = "\"" + passphrase + "\"";
            conf.priority = 1;
            mainWifi.addNetwork(conf);
            mainWifi.saveConfiguration();
            list = mainWifi.getConfiguredNetworks();
            for( WifiConfiguration i : list ) {
                if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                    mainWifi.enableNetwork(i.networkId, attemptConnect);
                    mainWifi.reconnect();
                    return;
                }
            }
        } catch(Exception e) {
           Log.e(TAG, "Exception bindSSID()");
        }
    }

    //-------------------------------------------------------------------------
    public static void startWifiScan() {
        Log.i(TAG, "Start WiFi Scan");
        receiverMode = ReceiverMode.SCANNING;
        mainWifi.startScan();
    }

    //-------------------------------------------------------------------------
    public static String connectedSSID() {
        return currentConnection;
    }

    //-------------------------------------------------------------------------
    public static void enableWiFi() {
        mainWifi.setWifiEnabled(true);
    }

    public static void disableWiFi() {
        mainWifi.setWifiEnabled(false);
    }

    //-------------------------------------------------------------------------
    class WifiReceiver extends BroadcastReceiver {
        public void onReceive(Context c, Intent intent) {
            try {
                if(intent.getAction().equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {
                    if(receiverMode == ReceiverMode.SCANNING) {
                        List<ScanResult> wifiList = mainWifi.getScanResults();
                        if (wifiList != null) {
                            for(int i = 0; i < wifiList.size(); i++) {
                                //Log.i(TAG, "SSID: " + wifiList.get(i).SSID + " | " + wifiList.get(i).capabilities + " | " + wifiList.get(i).frequency + " | " + wifiList.get(i).level);
                                nativeNewWifiItem(wifiList.get(i).SSID, wifiList.get(i).level);
                            }
                        }
                        nativeScanComplete();
                    }
                } else if (intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
                    NetworkInfo info = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                    if (info != null) {
                        NetworkInfo.DetailedState state = info.getDetailedState();
                        if (info.isConnectedOrConnecting() && info.isConnected()) {
                            if (state.compareTo(NetworkInfo.DetailedState.CONNECTED) == 0) {
                                WifiInfo wifiInfo = intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
                                if (wifiInfo != null) {
                                    Log.i(TAG, "Connected: " + wifiInfo);
                                    currentConnection = wifiInfo.getSSID();
                                    if(receiverMode == ReceiverMode.BINDING) {
                                        receiverMode = ReceiverMode.DISABLED;
                                    }
                                    currentWifiRssi = wifiInfo.getRssi();
                                    nativeNewWifiRSSI();
                                    nativeWifiConnected();
                                }
                            }
                        } else if (state.compareTo(NetworkInfo.DetailedState.DISCONNECTED) == 0) {
                            Log.i(TAG, "WIFI Disconnected");
                            currentWifiRssi = 0;
                            currentConnection = "";
                            nativeNewWifiRSSI();
                            nativeWifiDisconnected();
                        }
                    }
                } else if (intent.getAction().equals(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION)) {
                    if(receiverMode == ReceiverMode.BINDING) {
                        int error = intent.getIntExtra(WifiManager.EXTRA_SUPPLICANT_ERROR, 0);
                        if (error == WifiManager.ERROR_AUTHENTICATING) {
                            Log.e(TAG, "Authentication Error");
                            receiverMode = ReceiverMode.DISABLED;
                            currentConnection = "";
                            if(lastAttemptedSSID != null && !lastAttemptedSSID.isEmpty()) {
                                forgetSSID(lastAttemptedSSID);
                                nativeAuthError();
                            }
                        }
                    }
                } else if (intent.getAction().equals(WifiManager.RSSI_CHANGED_ACTION)) {
                    WifiInfo wifiInfo = mainWifi.getConnectionInfo();
                    Log.e(TAG, "RSSI: " + wifiInfo.getRssi());
                    currentWifiRssi = wifiInfo.getRssi();
                    nativeNewWifiRSSI();
                } else if (intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
                    int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                    int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
                    batteryLevel = level / (float)scale;
                    Log.e(TAG, "Battery: " + batteryLevel);
                    nativeBatteryUpdate();
                }
            } catch(Exception e) {
            }
        }
    }
}

