package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import androidx.core.content.ContextCompat;
import androidx.core.content.IntentCompat;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;

/** Owns {@code ACTION_USB_DEVICE_ATTACHED}/{@code DETACHED}, kept separate from the permission receiver in {@link QGCUsbSerialManager} so the permission and device-presence flows have independent receivers. */
final class UsbAttachDetachReceiver {

    private static final String TAG = UsbAttachDetachReceiver.class.getSimpleName();

    private final Consumer<UsbDevice> onAttached;
    private final Consumer<UsbDevice> onDetached;
    private final AtomicBoolean registered = new AtomicBoolean(false);
    private Context appContext;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, final Intent intent) {
            final String action = intent.getAction();
            if (action == null) return;
            final UsbDevice device = IntentCompat.getParcelableExtra(
                    intent, UsbManager.EXTRA_DEVICE, UsbDevice.class);
            if (device == null) return;
            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                onAttached.accept(device);
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                onDetached.accept(device);
            }
        }
    };

    UsbAttachDetachReceiver(final Consumer<UsbDevice> onAttached, final Consumer<UsbDevice> onDetached) {
        this.onAttached = onAttached;
        this.onDetached = onDetached;
    }

    void register(final Context context) {
        if (!registered.compareAndSet(false, true)) return;
        appContext = context.getApplicationContext();
        final IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        try {
            // System-delivered protected broadcasts; NOT_EXPORTED stops other apps forging a USB attach/detach to DoS the link. ContextCompat maps this to no flag pre-API 33.
            ContextCompat.registerReceiver(appContext, receiver, filter, ContextCompat.RECEIVER_NOT_EXPORTED);
            QGCLogger.i(TAG, "Attach/detach receiver registered.");
        } catch (final Exception e) {
            registered.set(false);
            appContext = null;
            QGCLogger.e(TAG, "Failed to register attach/detach receiver", e);
        }
    }

    void unregister() {
        if (!registered.compareAndSet(true, false)) return;
        try {
            if (appContext != null) appContext.unregisterReceiver(receiver);
        } catch (final IllegalArgumentException e) {
            QGCLogger.w(TAG, "Receiver not registered: " + e.getMessage());
        } finally {
            appContext = null;
        }
    }
}
