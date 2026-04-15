package org.mavlink.qgroundcontrol;

import android.app.PendingIntent;
import android.content.*;
import android.hardware.usb.*;
import android.os.Build;

/**
 * Handles USB permission requests and USB attach/detach broadcasts on behalf of
 * {@link QGCUsbSerialManager}.  Owns the {@link BroadcastReceiver} lifecycle and
 * the {@link PendingIntent} used for permission dialogs.
 */
class QGCUsbPermissionHandler {

    private static final String TAG = QGCUsbPermissionHandler.class.getSimpleName();
    static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";

    /**
     * Callback interface for USB device events.  Implemented by
     * {@link QGCUsbSerialManager}.
     */
    interface Listener {
        void onUsbDeviceAttached(UsbDevice device);
        void onUsbDeviceDetached(UsbDevice device);
        void onUsbPermissionGranted(UsbDevice device);
        void onUsbPermissionDenied(UsbDevice device);
    }

    private final Listener listener;
    private PendingIntent usbPermissionIntent;
    private boolean receiverRegistered;

    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, final Intent intent) {
            final String action = intent.getAction();
            QGCLogger.i(TAG, "BroadcastReceiver USB action " + action);
            if (action == null) {
                return;
            }

            switch (action) {
                case ACTION_USB_PERMISSION:
                    handleUsbPermission(intent);
                    break;
                case UsbManager.ACTION_USB_DEVICE_DETACHED:
                    handleUsbDeviceDetached(intent);
                    break;
                case UsbManager.ACTION_USB_DEVICE_ATTACHED:
                    handleUsbDeviceAttached(intent);
                    break;
                default:
                    break;
            }
        }
    };

    QGCUsbPermissionHandler(final Listener listener) {
        this.listener = listener;
    }

    /**
     * Builds the {@link PendingIntent} and registers the receiver.
     * Safe to call repeatedly — no-op if already registered.
     */
    void register(final Context context) {
        if (receiverRegistered) {
            return;
        }

        final Intent permissionIntent =
                new Intent(ACTION_USB_PERMISSION).setPackage(context.getPackageName());
        int flags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            flags |= PendingIntent.FLAG_IMMUTABLE;
        }
        usbPermissionIntent = PendingIntent.getBroadcast(context, 0, permissionIntent, flags);

        final IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                context.registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
            } else {
                context.registerReceiver(usbReceiver, filter);
            }
            receiverRegistered = true;
            QGCLogger.i(TAG, "BroadcastReceiver registered successfully.");
        } catch (final Exception e) {
            receiverRegistered = false;
            QGCLogger.e(TAG, "Failed to register BroadcastReceiver", e);
        }
    }

    /**
     * Unregisters the receiver and releases the {@link PendingIntent}.
     * Safe to call when not registered.
     */
    void unregister(final Context context) {
        if (!receiverRegistered) {
            return;
        }
        try {
            context.unregisterReceiver(usbReceiver);
            QGCLogger.i(TAG, "BroadcastReceiver unregistered successfully.");
        } catch (final IllegalArgumentException e) {
            QGCLogger.w(TAG, "Receiver not registered: " + e.getMessage());
        } finally {
            receiverRegistered = false;
            usbPermissionIntent = null;
        }
    }

    /**
     * Requests USB permission for {@code device}.  Callers should check
     * {@link UsbManager#hasPermission} before calling this.
     */
    void requestPermission(final UsbManager usbManager, final UsbDevice device) {
        if (usbPermissionIntent == null) {
            QGCLogger.e(TAG, "requestPermission called before register()");
            return;
        }
        usbManager.requestPermission(device, usbPermissionIntent);
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    private void handleUsbPermission(final Intent intent) {
        final UsbDevice device = getUsbDevice(intent);
        if (device == null) {
            return;
        }
        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
            QGCLogger.i(TAG, "Permission granted to " + device.getDeviceName());
            listener.onUsbPermissionGranted(device);
        } else {
            QGCLogger.i(TAG, "Permission denied for " + device.getDeviceName());
            listener.onUsbPermissionDenied(device);
        }
    }

    private void handleUsbDeviceDetached(final Intent intent) {
        final UsbDevice device = getUsbDevice(intent);
        if (device != null) {
            listener.onUsbDeviceDetached(device);
        }
    }

    private void handleUsbDeviceAttached(final Intent intent) {
        final UsbDevice device = getUsbDevice(intent);
        if (device != null) {
            listener.onUsbDeviceAttached(device);
        }
    }

    @SuppressWarnings("deprecation")
    private static UsbDevice getUsbDevice(final Intent intent) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
        }
        return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
    }
}
