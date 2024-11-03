package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.view.WindowManager;

import org.qtproject.qt.android.bindings.QtActivity;

public class QGCActivity extends QtActivity {
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String SCREEN_BRIGHT_WAKE_LOCK_TAG = "QGroundControl";
    private static final String MULTICAST_LOCK_TAG = "QGroundControl";

    private static QGCActivity m_instance = null;

    private PowerManager.WakeLock m_wakeLock;
    private WifiManager.MulticastLock m_wifiMulticastLock;

    public QGCActivity() {
        m_instance = this;
    }

    /**
     * Returns the singleton instance of QGCActivity.
     *
     * @return The current instance of QGCActivity.
     */
    public static QGCActivity getInstance() {
        return m_instance;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeInit();
        acquireWakeLock();
        keepScreenOn();
        setupMulticastLock();

        QGCUsbSerialManager.initialize(this);
    }

    @Override
    protected void onDestroy() {
        try {
            releaseMulticastLock();
            releaseWakeLock();
            QGCUsbSerialManager.cleanup(this);
        } catch (final Exception e) {
            Log.e(TAG, "Exception onDestroy()", e);
        }

        super.onDestroy();
    }

    /**
     * Keeps the screen on by adding the appropriate window flag.
     */
    private void keepScreenOn() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    /**
     * Acquires a wake lock to keep the CPU running.
     */
    private void acquireWakeLock() {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        m_wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, SCREEN_BRIGHT_WAKE_LOCK_TAG);
        if (m_wakeLock != null) {
            m_wakeLock.acquire();
        } else {
            Log.w(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!");
        }
    }

    /**
     * Releases the wake lock if held.
     */
    private void releaseWakeLock() {
        if (m_wakeLock != null && m_wakeLock.isHeld()) {
            m_wakeLock.release();
        }
    }

    /**
     * Sets up a multicast lock to allow multicast packets.
     */
    private void setupMulticastLock() {
        if (m_wifiMulticastLock == null) {
            final WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
            m_wifiMulticastLock = wifi.createMulticastLock(MULTICAST_LOCK_TAG);
            m_wifiMulticastLock.setReferenceCounted(true);
        }

        m_wifiMulticastLock.acquire();
        Log.d(TAG, "Multicast lock: " + m_wifiMulticastLock.toString());
    }

    /**
     * Releases the multicast lock if held.
     */
    private void releaseMulticastLock() {
        if (m_wifiMulticastLock != null && m_wifiMulticastLock.isHeld()) {
            m_wifiMulticastLock.release();
            Log.d(TAG, "Multicast lock released.");
        }
    }

    // Native C++ functions
    public native boolean nativeInit();
    public native void qgcLogDebug(final String message);
    public native void qgcLogWarning(final String message);
}
