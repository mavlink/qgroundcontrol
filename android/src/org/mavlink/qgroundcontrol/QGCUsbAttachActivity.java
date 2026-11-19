package org.mavlink.qgroundcontrol;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import androidx.core.content.IntentCompat;

import org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager;

/**
 * Takes the intent that Android sends once the user has let QGC open a USB device that was just attached, and
 * finishes at once without showing anything.
 *
 * <p>{@link QGCActivity} must not take that intent itself. Android hands an intent to the QGCActivity that
 * exists only while it is on top of its task, and makes a second one otherwise, as while the file picker is
 * open over it; Qt then restarts the whole app, which drops every link. This activity passes the device on to
 * the serial code and shows QGC as it is. If QGC is not running, it starts it, and QGC finds the device and its
 * permission as it starts.
 */
public final class QGCUsbAttachActivity extends Activity {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbAttachActivity";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            if (QGCActivity.isRunning()) {
                // Only now: the serial code tells C++, whose library is loaded once QGCActivity exists.
                final Intent intent = getIntent();
                if (intent != null && UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(intent.getAction())) {
                    QGCUsbSerialManager.onUsbDeviceGranted(
                            IntentCompat.getParcelableExtra(intent, UsbManager.EXTRA_DEVICE, UsbDevice.class));
                }
                showQgcTask();
            } else {
                final Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
                if (launch != null) {
                    startActivity(launch);
                }
            }
        } catch (final RuntimeException e) {
            QGCLogger.e(TAG, "Failed to pass on an attached USB device", e);
        }
        // The theme shows nothing, which Android allows only to an activity that finishes here.
        finish();
    }

    /**
     * Brings the task of the running QGC to the front as it is. Not by an intent, which would make that second
     * QGCActivity.
     */
    private void showQgcTask() {
        final ComponentName mainActivity = new ComponentName(this, QGCActivity.class);
        final ActivityManager activityManager = getSystemService(ActivityManager.class);
        if (activityManager == null) {
            return;
        }
        for (final ActivityManager.AppTask task : activityManager.getAppTasks()) {
            if (mainActivity.equals(task.getTaskInfo().baseActivity)) {
                task.moveToFront();
                return;
            }
        }
    }
}
