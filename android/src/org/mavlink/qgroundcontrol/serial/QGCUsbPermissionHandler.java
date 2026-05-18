package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import androidx.core.content.ContextCompat;
import androidx.core.content.IntentCompat;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Handles USB permission requests and USB attach/detach broadcasts on behalf of
 * {@link QGCUsbSerialManager}.
 *
 * <p>Android has no {@code ActivityResultContract} for USB host permissions; the
 * {@code UsbManager.requestPermission} API is inherently BroadcastReceiver-based
 * through at least API 34. {@link ContextCompat#registerReceiver} handles the
 * {@code RECEIVER_NOT_EXPORTED} flag without a hand-rolled SDK check.</p>
 */
class QGCUsbPermissionHandler {

    private static final String TAG = QGCUsbPermissionHandler.class.getSimpleName();
    static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";

    /** Per-physical-device permission state. Owned here so callers don't need parallel sets. */
    enum PermissionStatus { PENDING, GRANTED, DENIED }

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
    /** physicalDeviceId → status. Cleared per-device on detach so re-attach can re-prompt. */
    private final Map<Integer, PermissionStatus> permissionStatus = new ConcurrentHashMap<>();

    /** Guards {@link #usbPermissionIntent}, {@link #registered}, and {@link #appContext}. */
    private final ReentrantLock lock = new ReentrantLock();
    /** Fast-path idempotency check that avoids acquiring the lock for every call. */
    private final AtomicBoolean registered = new AtomicBoolean(false);
    private PendingIntent usbPermissionIntent;
    /** Captured at register() so unregister() always uses the same Application context. */
    private Context appContext;
    /** Most recent device passed to {@link #requestPermission}; used as fallback when
     *  the OEM-broken ACTION_USB_PERMISSION arrives without EXTRA_DEVICE. */
    private volatile UsbDevice lastRequestedDevice;
    /** Captured alongside {@link #lastRequestedDevice} for the same fallback path. */
    private volatile UsbManager lastUsbManager;

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
        // Fast path: avoid lock acquisition when already registered.
        if (registered.get()) {
            return;
        }

        lock.lock();
        try {
            if (registered.get()) {
                return;
            }

            final Context ctx = context.getApplicationContext();
            final Intent permissionIntent =
                    new Intent(ACTION_USB_PERMISSION).setPackage(ctx.getPackageName());
            final int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
            usbPermissionIntent = PendingIntent.getBroadcast(ctx, 0, permissionIntent, flags);

            final IntentFilter filter = new IntentFilter();
            filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
            filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
            filter.addAction(ACTION_USB_PERMISSION);

            try {
                ContextCompat.registerReceiver(
                        ctx,
                        usbReceiver,
                        filter,
                        ContextCompat.RECEIVER_NOT_EXPORTED);
                appContext = ctx;
                registered.set(true);
                QGCLogger.i(TAG, "BroadcastReceiver registered successfully.");
            } catch (final Exception e) {
                usbPermissionIntent = null;
                QGCLogger.e(TAG, "Failed to register BroadcastReceiver", e);
            }
        } finally {
            lock.unlock();
        }
    }

    /**
     * Unregisters the receiver and releases the {@link PendingIntent}.
     * Safe to call when not registered. Uses the Application context captured
     * during {@link #register} so it cannot mismatch the registration context.
     */
    void unregister() {
        if (!registered.get()) {
            return;
        }

        lock.lock();
        try {
            if (!registered.get()) {
                return;
            }
            try {
                if (appContext != null) {
                    appContext.unregisterReceiver(usbReceiver);
                    QGCLogger.i(TAG, "BroadcastReceiver unregistered successfully.");
                }
            } catch (final IllegalArgumentException e) {
                QGCLogger.w(TAG, "Receiver not registered: " + e.getMessage());
            } finally {
                registered.set(false);
                usbPermissionIntent = null;
                appContext = null;
                lastRequestedDevice = null;
                lastUsbManager = null;
                permissionStatus.clear();
            }
        } finally {
            lock.unlock();
        }
    }

    /**
     * Requests USB permission for {@code device}.  Callers should check
     * {@link UsbManager#hasPermission} before calling this.
     */
    void requestPermission(final UsbManager usbManager, final UsbDevice device) {
        final PendingIntent intent;
        lock.lock();
        try {
            intent = usbPermissionIntent;
        } finally {
            lock.unlock();
        }
        if (intent == null) {
            QGCLogger.e(TAG, "requestPermission called before register()");
            return;
        }
        lastRequestedDevice = device;
        lastUsbManager = usbManager;
        permissionStatus.put(device.getDeviceId(), PermissionStatus.PENDING);
        usbManager.requestPermission(device, intent);
    }

    /** Returns true if the user previously denied permission for {@code physicalDeviceId}
     *  this attach cycle. Cleared by {@link #clearDevice} on detach / re-grant. */
    boolean isDenied(final int physicalDeviceId) {
        return permissionStatus.get(physicalDeviceId) == PermissionStatus.DENIED;
    }

    /** Drops any cached permission state for {@code physicalDeviceId}; call on detach
     *  so the next physical re-attach can re-prompt the user. */
    void clearDevice(final int physicalDeviceId) {
        permissionStatus.remove(physicalDeviceId);
    }

    private void handleUsbPermission(final Intent intent) {
        final boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
        UsbDevice device = getUsbDevice(intent);
        boolean actuallyGranted = granted;
        if (device == null) {
            // Android-15 OEM quirk: ACTION_USB_PERMISSION delivered with no EXTRA_DEVICE and
            // granted=false regardless of user choice. Fall back to the most recent request
            // and the authoritative hasPermission() state.
            final UsbDevice pending = lastRequestedDevice;
            final UsbManager mgr = lastUsbManager;
            if (pending == null || mgr == null) {
                QGCLogger.w(TAG, "ACTION_USB_PERMISSION with no EXTRA_DEVICE and no pending request");
                return;
            }
            device = pending;
            actuallyGranted = mgr.hasPermission(device);
            QGCLogger.w(TAG, "ACTION_USB_PERMISSION malformed; resolved " + device.getDeviceName()
                    + " via hasPermission()=" + actuallyGranted);
        }
        if (device.equals(lastRequestedDevice)) {
            lastRequestedDevice = null;
        }
        if (actuallyGranted) {
            permissionStatus.put(device.getDeviceId(), PermissionStatus.GRANTED);
            QGCLogger.i(TAG, "Permission granted to " + device.getDeviceName());
            listener.onUsbPermissionGranted(device);
        } else {
            permissionStatus.put(device.getDeviceId(), PermissionStatus.DENIED);
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

    private static UsbDevice getUsbDevice(final Intent intent) {
        return IntentCompat.getParcelableExtra(intent, UsbManager.EXTRA_DEVICE, UsbDevice.class);
    }
}
