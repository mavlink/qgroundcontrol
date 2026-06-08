package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import androidx.annotation.MainThread;
import androidx.core.content.ContextCompat;
import androidx.core.content.IntentCompat;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Consumer;

/** Owns the USB permission PendingIntent, receiver, and per-device permission state. */
final class UsbPermissionManager {

    private static final String TAG = UsbPermissionManager.class.getSimpleName();

    private static final String ACTION_USB_PERMISSION =
            "org.mavlink.qgroundcontrol.action.USB_PERMISSION";

    enum PermissionStatus { PENDING, GRANTED, DENIED }

    private final Consumer<UsbDevice> onGranted;
    private final Consumer<UsbDevice> onDenied;

    private final Map<String, PermissionStatus> permissionStatus = new ConcurrentHashMap<>();
    /** Guards {@link #usbPermissionIntent} and the registered flag. */
    private final Object permissionLock = new Object();
    private volatile boolean permissionRegistered;
    private PendingIntent usbPermissionIntent;

    private record PermissionRequest(UsbDevice device, UsbManager manager) {}
    /** Pending requests keyed by device name so concurrent attaches don't clobber each other; the malformed-broadcast fallback resolves only when exactly one request is pending. */
    private final Map<String, PermissionRequest> pendingRequests = new ConcurrentHashMap<>();

    private Context appContext;

    private final BroadcastReceiver usbPermissionReceiver = new BroadcastReceiver() {
        @Override
        @MainThread
        public void onReceive(final Context context, final Intent intent) {
            final String action = intent.getAction();
            QGCLogger.i(TAG, "BroadcastReceiver USB action " + action);
            if (action == null) {
                return;
            }
            if (ACTION_USB_PERMISSION.equals(action)) {
                handleUsbPermissionResult(intent);
            }
        }
    };

    UsbPermissionManager(final Consumer<UsbDevice> onGranted, final Consumer<UsbDevice> onDenied) {
        this.onGranted = onGranted;
        this.onDenied = onDenied;
    }

    /** Builds the {@link PendingIntent} and registers the USB permission receiver; safe to call repeatedly (no-op if already registered). */
    void register(final Context context) {
        synchronized (permissionLock) {
            if (permissionRegistered) {
                return;
            }

            final Context ctx = context.getApplicationContext();
            appContext = ctx;
            final Intent permissionIntent = new Intent(ACTION_USB_PERMISSION).setPackage(ctx.getPackageName());
            // API 31+: immutable is safe — framework populates EXTRA_DEVICE / EXTRA_PERMISSION_GRANTED via the delivery mechanism, not by mutating the intent.
            final int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
            usbPermissionIntent = PendingIntent.getBroadcast(ctx, 0, permissionIntent, flags);

            final IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);

            try {
                ContextCompat.registerReceiver(
                        ctx,
                        usbPermissionReceiver,
                        filter,
                        ContextCompat.RECEIVER_NOT_EXPORTED);
                permissionRegistered = true;
                QGCLogger.i(TAG, "USB permission BroadcastReceiver registered successfully.");
            } catch (final Exception e) {
                usbPermissionIntent = null;
                appContext = null;
                QGCLogger.e(TAG, "Failed to register USB permission BroadcastReceiver", e);
            }
        }
    }

    /** Unregisters the USB permission receiver and releases the {@link PendingIntent}; safe to call when not registered. */
    void unregister() {
        synchronized (permissionLock) {
            if (!permissionRegistered) {
                return;
            }
            try {
                if (appContext != null) appContext.unregisterReceiver(usbPermissionReceiver);
                QGCLogger.i(TAG, "USB permission BroadcastReceiver unregistered successfully.");
            } catch (final IllegalArgumentException e) {
                QGCLogger.w(TAG, "Permission receiver not registered: " + e.getMessage());
            } finally {
                // Flip registered=false first so an in-flight handleUsbPermissionResult bails at its registered-gate before reading the maps we clear below.
                permissionRegistered = false;
                usbPermissionIntent = null;
                pendingRequests.clear();
                permissionStatus.clear();
                appContext = null;
            }
        }
    }

    /** Requests USB permission for {@code device}; callers should check {@link UsbManager#hasPermission} first. */
    void request(final UsbManager mgr, final UsbDevice device) {
        final PendingIntent intent;
        synchronized (permissionLock) {
            intent = usbPermissionIntent;
        }
        if (intent == null) {
            QGCLogger.e(TAG, "requestUsbPermission called before registerPermissionReceiver()");
            return;
        }
        pendingRequests.put(device.getDeviceName(), new PermissionRequest(device, mgr));
        permissionStatus.put(device.getDeviceName(), PermissionStatus.PENDING);
        mgr.requestPermission(device, intent);
    }

    /** Returns true if the user previously denied permission for {@code deviceName} this attach cycle. */
    boolean isPermissionDenied(final String deviceName) {
        return permissionStatus.get(deviceName) == PermissionStatus.DENIED;
    }

    /**
     * Drops cached permission status for {@code deviceName} on detach so re-attach can re-prompt. Deliberately leaves
     * {@link #pendingRequests} untouched: detach runs on the USB worker while the permission result lands on the main
     * looper with no shared lock, so a transient detach (hub blip / OTG jiggle) racing the dialog must not drop the
     * in-flight request — else the malformed-broadcast fallback loses the grant when the user taps Allow. A stale entry
     * left behind by a detach is tolerated by {@link #resolveMalformedTarget}; it self-clears via
     * {@link #handleUsbPermissionResult}, a same-key re-request, or {@link #unregister}.
     */
    void clearPermission(final String deviceName) {
        permissionStatus.remove(deviceName);
    }

    /**
     * Picks the target for a malformed (no-EXTRA_DEVICE) broadcast: the sole outstanding request, or — when several are
     * pending because a stale entry survived a detach ({@link #clearPermission} keeps {@link #pendingRequests}) — the
     * unique device the framework just flipped to {@code hasPermission()==true}. Returns null when the choice is
     * genuinely ambiguous (zero pending, or more than one currently granted).
     */
    private PermissionRequest resolveMalformedTarget() {
        if (pendingRequests.size() == 1) {
            return pendingRequests.values().iterator().next();
        }
        PermissionRequest granted = null;
        for (final PermissionRequest req : pendingRequests.values()) {
            if (req.manager().hasPermission(req.device())) {
                if (granted != null) {
                    return null;
                }
                granted = req;
            }
        }
        return granted;
    }

    private void handleUsbPermissionResult(final Intent intent) {
        // Ignore broadcasts arriving after unregister() (teardown) — they'd resurrect a request.
        if (!permissionRegistered) {
            QGCLogger.w(TAG, "USB permission result after unregister; ignoring");
            return;
        }
        final boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
        UsbDevice device = IntentCompat.getParcelableExtra(intent, UsbManager.EXTRA_DEVICE, UsbDevice.class);
        boolean actuallyGranted = granted;
        if (device == null) {
            // Android-15 OEM quirk: ACTION_USB_PERMISSION delivered with no EXTRA_DEVICE and granted=false regardless of choice.
            final PermissionRequest pending = resolveMalformedTarget();
            if (pending == null) {
                QGCLogger.w(TAG, "ACTION_USB_PERMISSION malformed with " + pendingRequests.size()
                        + " pending requests; cannot resolve target device");
                return;
            }
            device = pending.device();
            actuallyGranted = pending.manager().hasPermission(device);
            QGCLogger.w(TAG, "ACTION_USB_PERMISSION malformed; resolved " + device.getDeviceName()
                    + " via hasPermission()=" + actuallyGranted);
        }
        pendingRequests.remove(device.getDeviceName());
        if (actuallyGranted) {
            permissionStatus.put(device.getDeviceName(), PermissionStatus.GRANTED);
            QGCLogger.i(TAG, "Permission granted to " + device.getDeviceName());
            onGranted.accept(device);
        } else {
            permissionStatus.put(device.getDeviceName(), PermissionStatus.DENIED);
            QGCLogger.i(TAG, "Permission denied for " + device.getDeviceName());
            onDenied.accept(device);
        }
    }
}
