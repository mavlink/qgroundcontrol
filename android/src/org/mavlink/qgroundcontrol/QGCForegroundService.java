package org.mavlink.qgroundcontrol;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;

import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.ServiceCompat;

/**
 * Keeps QGroundControl's process in the foreground state while native code asks for it: with a vehicle connected
 * or a video being recorded. Android then neither freezes the process nor cuts its network once the user leaves
 * the app, and the service's notification leads back to it.
 */
public class QGCForegroundService extends Service {
    private static final String TAG = QGCForegroundService.class.getSimpleName();
    private static final String CHANNEL_ID = "background";
    private static final int NOTIFICATION_ID = 1;

    /** The notification's texts, in the app's language. */
    static final class Texts {
        final String channelName;
        final String title;
        final String text;

        Texts(final String channelName, final String title, final String text) {
            this.channelName = channelName;
            this.title = title;
            this.text = text;
        }
    }

    /** What native code last asked for: null when it wants no service. Set on Qt's thread. */
    private static volatile Texts s_wanted = null;

    static void setWanted(final Texts texts) {
        s_wanted = texts;
    }

    /** Starts or stops the service to match what native code last asked for. Android's main thread. */
    static void sync(final Context context) {
        final Intent intent = new Intent(context, QGCForegroundService.class);
        if (s_wanted == null) {
            context.stopService(intent);
            return;
        }
        try {
            // Not startForegroundService(): Android refuses both to an app in the background, and that one
            // crashes the app when the service is stopped before it has started.
            context.startService(intent);
        } catch (final IllegalStateException e) {
            QGCLogger.i(TAG, "Not started while in the background; QGCActivity.onResume() asks again");
        }
    }

    @Override
    public int onStartCommand(final Intent intent, final int flags, final int startId) {
        final Texts texts = s_wanted;
        if (texts == null) {
            stopSelf();
            return START_NOT_STICKY;
        }
        try {
            // Android 14+ requires the type, which exists from Android 10 on.
            final int type = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
                    ? ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE : 0;
            ServiceCompat.startForeground(this, NOTIFICATION_ID, buildNotification(texts), type);
        } catch (final RuntimeException e) {
            // Android 12+ refuses it once the app has left the foreground: QGCActivity.onResume() asks again.
            QGCLogger.w(TAG, "Could not enter the foreground state: " + e);
            stopSelf();
        }
        // The service means nothing to a process Android has restarted without the app.
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(final Intent intent) {
        return null;
    }

    private Notification buildNotification(final Texts texts) {
        NotificationManagerCompat.from(this).createNotificationChannel(
                new NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
                        .setName(texts.channelName)
                        .build());

        final NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(smallIcon())
                .setContentTitle(texts.title)
                .setContentText(texts.text)
                .setOngoing(true)
                .setCategory(NotificationCompat.CATEGORY_SERVICE)
                .setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE);

        final Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
        if (launch != null) {
            builder.setContentIntent(PendingIntent.getActivity(this, 0, launch,
                    PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT));
        }
        return builder.build();
    }

    /** The launcher icon's foreground, the logo alone, which Android shows as its outline. */
    @SuppressLint("DiscouragedApi") // R is generated in the application id's package, which custom builds change.
    private int smallIcon() {
        final int logo = getResources().getIdentifier("ic_launcher_foreground", "drawable", getPackageName());
        return (logo != 0) ? logo : getApplicationInfo().icon;
    }
}
