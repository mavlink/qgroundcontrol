package org.qgroundcontrol.qgchelper;

import android.app.Activity;
import android.util.Log;
import android.os.PowerManager;
import android.content.Context;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.view.WindowManager;

import java.util.List;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;

import org.qtproject.qt5.android.bindings.QtActivity;
import org.qtproject.qt5.android.bindings.QtApplication;

public class UsbDeviceJNI extends QtActivity implements TextToSpeech.OnInitListener {

    private static UsbDeviceJNI m_instance;
    private static final String TAG = "QGroundControl_JNI";
    private static TextToSpeech  m_tts;
    private static PowerManager.WakeLock m_wl;
    private static WifiManager mainWifi;
    private static WifiReceiver receiverWifi;

    // WiFi: https://stackoverflow.com/questions/36098871/how-to-search-and-connect-to-a-specific-wifi-network-in-android-programmatically/36099552#36099552

    private static native void nativeNewWifiItem(String messageA);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Constructor.  Only used once to create the initial instance for the static functions.
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////
    public UsbDeviceJNI() {
        m_instance = this;
        Log.i(TAG, "Instance created");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize TTS and Power Management
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate()");
        super.onCreate(savedInstanceState);
        m_tts = new TextToSpeech(this,this);
        PowerManager pm = (PowerManager)m_instance.getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "QGroundControl");
        if(m_wl != null) {
            m_wl.acquire();
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK acquired.");
        } else {
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!!!");
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        //-- WiFi
        mainWifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        receiverWifi = new WifiReceiver();
        if(mainWifi.isWifiEnabled() == false) {
            mainWifi.setWifiEnabled(true);
        }
    }

    @Override
    protected void onDestroy() {
        if(m_wl != null) {
            m_wl.release();
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK released.");
        }
        super.onDestroy();
        m_tts.shutdown();
    }

    public void onInit(int status) {
    }

    public static void say(String msg) {
        Log.i(TAG, "Say: " + msg);
        m_tts.speak(msg, TextToSpeech.QUEUE_FLUSH, null);
    }

    public static void keepScreenOn() {
    }

    public static void restoreScreenOn() {
    }

    public static void bindSSID(String ssid) {
        Log.i(TAG, "Bind: " + ssid);
        List<WifiConfiguration> list = mainWifi.getConfiguredNetworks();
        //-- Disable everything else
        mainWifi.disconnect();
        for( WifiConfiguration i : list ) {
            if(i.SSID != null && !i.SSID.equals("\"" + ssid + "\"")) {
                mainWifi.disableNetwork(i.networkId);
            }
        }
        mainWifi.saveConfiguration();
        //-- If already set, just make sure it's enabled
        list = mainWifi.getConfiguredNetworks();
        for( WifiConfiguration i : list ) {
            if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                mainWifi.enableNetwork(i.networkId, true);
                mainWifi.reconnect();
                return;
            }
        }
        //-- Add new configuration
        WifiConfiguration conf = new WifiConfiguration();
        conf.SSID = "\"" + ssid + "\"";
        conf.preSharedKey = "\"1234567890\"";
        mainWifi.addNetwork(conf);
        mainWifi.saveConfiguration();
        list = mainWifi.getConfiguredNetworks();
        for( WifiConfiguration i : list ) {
            if(i.SSID != null && i.SSID.equals("\"" + ssid + "\"")) {
                mainWifi.enableNetwork(i.networkId, true);
                mainWifi.reconnect();
                return;
            }
        }
    }

    public static boolean isWIFIConnected() {
        ConnectivityManager connManager = (ConnectivityManager) m_instance.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo mWifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        return mWifi.isConnected();
    }

    public static void startWifiScan() {
        Log.i(TAG, "Start WiFi Scan");
        m_instance.registerReceiver(receiverWifi, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
        mainWifi.startScan();
    }

    public static void stopWifiScan() {
        Log.i(TAG, "Stop WiFi Scan");
        m_instance.unregisterReceiver(receiverWifi);
    }

    class WifiReceiver extends BroadcastReceiver {
        public void onReceive(Context c, Intent intent) {
            List<ScanResult> wifiList = mainWifi.getScanResults();
            for(int i = 0; i < wifiList.size(); i++) {
                Log.i(TAG, "SSID: " + wifiList.get(i).SSID + " | " + wifiList.get(i).capabilities + " | " + wifiList.get(i).frequency + " | " + wifiList.get(i).level);
                nativeNewWifiItem(wifiList.get(i).SSID);
            }
        }
    }
}

