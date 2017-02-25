package org.qgroundcontrol.qgchelper;

import android.app.Activity;
import android.util.Log;
import android.os.PowerManager;
import android.content.Context;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.view.WindowManager;

import org.qtproject.qt5.android.bindings.QtActivity;
import org.qtproject.qt5.android.bindings.QtApplication;

public class UsbDeviceJNI extends QtActivity implements TextToSpeech.OnInitListener
{
    private static UsbDeviceJNI m_instance;
    private static final String TAG = "QGroundControl_JNI";
    private static TextToSpeech  m_tts;
    private static PowerManager.WakeLock m_wl;

    // WiFi: https://stackoverflow.com/questions/36098871/how-to-search-and-connect-to-a-specific-wifi-network-in-android-programmatically/36099552#36099552

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Constructor.  Only used once to create the initial instance for the static functions.
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////
    public UsbDeviceJNI()
    {
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

    public static void say(String msg)
    {
        Log.i(TAG, "Say: " + msg);
        m_tts.speak(msg, TextToSpeech.QUEUE_FLUSH, null);
    }

    public static void keepScreenOn()
    {
    }

    public static void restoreScreenOn()
    {
    }

}

