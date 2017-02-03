package org.qgroundcontrol.qgchelper;

import android.app.Activity;
import android.util.Log;
import android.os.PowerManager;
import android.content.Context;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;

import org.qtproject.qt5.android.bindings.QtActivity;
import org.qtproject.qt5.android.bindings.QtApplication;

public class UsbDeviceJNI extends QtActivity implements TextToSpeech.OnInitListener
{
    private static UsbDeviceJNI m_instance;
    private static final String TAG = "QGroundControl_JNI";
    private static TextToSpeech  m_tts;
    private static PowerManager.WakeLock m_wl;

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
        super.onCreate(savedInstanceState);
        m_tts = new TextToSpeech(this,this);
        PowerManager pm = (PowerManager)m_instance.getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "QGroundControl");
        Log.i(TAG, "onCreate()");
    }

    @Override
    protected void onDestroy() {
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
        if(m_wl != null) {
            m_wl.acquire();
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK acquired.");
        } else {
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!!!");
        }
    }

    public static void restoreScreenOn()
    {
        if(m_wl != null) {
            m_wl.release();
            Log.i(TAG, "SCREEN_BRIGHT_WAKE_LOCK released.");
        }
    }

}

