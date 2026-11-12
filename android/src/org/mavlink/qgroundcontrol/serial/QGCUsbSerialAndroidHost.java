package org.mavlink.qgroundcontrol.serial;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import androidx.core.content.ContextCompat;
import androidx.core.content.IntentCompat;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.util.List;

/**
 * Android's USB host API, as {@link QGCUsbSerialDevices} uses it, and the receiver of the USB broadcasts: a
 * device is attached or detached, or the user answers a permission request. Only this class deals with their
 * intents: it reports each as a {@link QGCUsbSerialEvent}. The host also keeps whether QGC's window has the focus,
 * as the activity reports it ({@link #setWindowFocused}).
 *
 * <p>The broadcasts are reported on a thread of the host's own, one at a time and in the order they arrive.
 * Android would deliver them on the UI thread, which must not wait for what handling one takes: calls into the
 * USB service and, on a detach, the closing of ports, which can wait for seconds behind a port that is being
 * opened.
 *
 * <p>The caller serializes {@link #start} and {@link #stop}. The {@link QGCUsbSerialHost} methods and
 * {@link #setWindowFocused} run on any thread, also after {@code stop}.
 */
final class QGCUsbSerialAndroidHost implements QGCUsbSerialHost {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialAndroidHost";
    static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";

    /**
     * Hears of each USB broadcast, on the host's event thread. {@code device} is null if the broadcast names
     * none.
     */
    @FunctionalInterface
    interface UsbEventListener {
        void onUsbEvent(QGCUsbSerialEvent event, UsbDevice device);
    }

    /** What a USB broadcast tells of: the event, and its device, or null if the broadcast names none. */
    record UsbBroadcast(QGCUsbSerialEvent event, UsbDevice device) {}

    private final Context _appContext;
    private final UsbManager _usbManager;
    private final PendingIntent _permissionIntent;
    private final UsbSerialProber _prober = QGCUsbSerialProber.getQGCUsbSerialProber();
    private final UsbEventListener _listener;
    // Handles the broadcasts. Null unless the receiver is registered.
    private HandlerThread _eventThread;
    // Whether QGC's window has the focus. Ports are opened on threads of C++, which must not ask the activity:
    // only the UI thread may touch its window.
    private volatile boolean _windowFocused;

    private final BroadcastReceiver _usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, final Intent intent) {
            final UsbBroadcast broadcast = decode(intent);
            if (broadcast == null) {
                // The filter lets no other action through.
                return;
            }
            final UsbDevice device = broadcast.device();
            QGCLogger.i(TAG, intent.getAction() + ": " + ((device != null) ? device.getDeviceName() : "no device"));
            reportEvent(_listener, broadcast.event(), device);
        }
    };

    /**
     * Returns what a broadcast tells of, or null if its action is none that the receiver registers for. An answer
     * to a permission request that does not say what the user chose counts as a refusal.
     */
    static UsbBroadcast decode(final Intent intent) {
        final QGCUsbSerialEvent event;
        switch (String.valueOf(intent.getAction())) {
            case UsbManager.ACTION_USB_DEVICE_ATTACHED -> event = QGCUsbSerialEvent.ATTACHED;
            case UsbManager.ACTION_USB_DEVICE_DETACHED -> event = QGCUsbSerialEvent.DETACHED;
            case ACTION_USB_PERMISSION -> event = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    ? QGCUsbSerialEvent.PERMISSION_GRANTED : QGCUsbSerialEvent.PERMISSION_DENIED;
            default -> {
                return null;
            }
        }
        return new UsbBroadcast(event,
                IntentCompat.getParcelableExtra(intent, UsbManager.EXTRA_DEVICE, UsbDevice.class));
    }

    /**
     * Reports an event to the listener. What the listener throws ends here, with a log line: nothing above the
     * receiver on the event thread would catch it, so it would end the app. The listener syncs with the attached
     * devices, where the library's prober passes on a driver that it could not create as a RuntimeException.
     */
    static void reportEvent(final UsbEventListener listener, final QGCUsbSerialEvent event, final UsbDevice device) {
        try {
            listener.onUsbEvent(event, device);
        } catch (final RuntimeException e) {
            QGCLogger.e(TAG, "Failed to handle USB event " + event, e);
        }
    }

    /**
     * Returns a host that reports the USB broadcasts to {@code listener} until {@link #stop}, or null if
     * Android has no USB service to offer.
     */
    static QGCUsbSerialAndroidHost start(final Context context, final UsbEventListener listener) {
        final Context appContext = context.getApplicationContext();
        final UsbManager usbManager = appContext.getSystemService(UsbManager.class);
        if (usbManager == null) {
            QGCLogger.e(TAG, "UsbManager unavailable");
            return null;
        }

        final QGCUsbSerialAndroidHost host = new QGCUsbSerialAndroidHost(appContext, usbManager, listener);
        host.registerReceiver();
        return host;
    }

    private QGCUsbSerialAndroidHost(final Context appContext, final UsbManager usbManager,
                                    final UsbEventListener listener) {
        _appContext = appContext;
        _usbManager = usbManager;
        _listener = listener;

        // UsbManager adds EXTRA_DEVICE and EXTRA_PERMISSION_GRANTED, so the intent must be mutable.
        final int mutableFlag = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) ? PendingIntent.FLAG_MUTABLE : 0;
        final Intent permissionAnswer = new Intent(ACTION_USB_PERMISSION).setPackage(appContext.getPackageName());
        _permissionIntent = PendingIntent.getBroadcast(appContext, 0, permissionAnswer, mutableFlag);
    }

    private void registerReceiver() {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);

        final HandlerThread eventThread = new HandlerThread("QGCUsbSerialEvents");
        eventThread.start();
        try {
            ContextCompat.registerReceiver(_appContext, _usbReceiver, filter, /* broadcastPermission */ null,
                    new Handler(eventThread.getLooper()), ContextCompat.RECEIVER_NOT_EXPORTED);
            _eventThread = eventThread;
        } catch (final RuntimeException e) {
            eventThread.quit();
            QGCLogger.e(TAG, "Failed to register USB receiver", e);
        }
    }

    /**
     * Stops reporting the USB broadcasts and ends the event thread, without waiting for it: the caller may be
     * the UI thread, and the broadcast that is being handled can take seconds. The listener still hears of that
     * one, possibly after this returns, but of none that wait behind it.
     */
    void stop() {
        if (_eventThread != null) {
            try {
                _appContext.unregisterReceiver(_usbReceiver);
            } catch (final IllegalArgumentException e) {
                QGCLogger.w(TAG, "USB receiver was not registered: " + e.getMessage());
            }
            // Not quit(), which discards the broadcasts still queued: Android no longer passes them to the
            // receiver, but it must run each to tell the system that it is done with it.
            _eventThread.quitSafely();
            _eventThread = null;
        }
    }

    @Override
    public List<UsbSerialDriver> findDrivers() {
        return _prober.findAllDrivers(_usbManager);
    }

    @Override
    public UsbSerialDriver probeDriver(final UsbDevice device) {
        return _prober.probeDevice(device);
    }

    @Override
    public boolean hasPermission(final UsbDevice device) {
        return _usbManager.hasPermission(device);
    }

    @Override
    public void requestPermission(final UsbDevice device) {
        _usbManager.requestPermission(device, _permissionIntent);
    }

    @Override
    public UsbDeviceConnection openDevice(final UsbDevice device) {
        return _usbManager.openDevice(device);
    }

    /** Notes whether QGC's window has the focus, as the activity is told on the UI thread. */
    void setWindowFocused(final boolean focused) {
        _windowFocused = focused;
    }

    @Override
    public boolean canAskUser() {
        return _windowFocused;
    }
}
