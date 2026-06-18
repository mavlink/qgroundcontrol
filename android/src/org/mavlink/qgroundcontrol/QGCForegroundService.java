package org.mavlink.qgroundcontrol;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;

/**
 * Foreground service that keeps a QGC background workload alive while the app is backgrounded.
 *
 * <p>Caller provides a {@link Config} naming the notification channel + text and the
 * {@link ServiceInfo} FGS type. The manifest entry's {@code android:foregroundServiceType}
 * must declare every FGS type passed in via {@link Config#foregroundServiceType()}.</p>
 */
public final class QGCForegroundService extends Service {

    private static final String ACTION_START = "org.mavlink.qgroundcontrol.action.START_FOREGROUND_SERVICE";

    private static final String EXTRA_FGS_TYPE         = "qgc.fgs.type";
    private static final String EXTRA_CHANNEL_ID       = "qgc.fgs.channelId";
    private static final String EXTRA_CHANNEL_NAME     = "qgc.fgs.channelName";
    private static final String EXTRA_CHANNEL_DESC     = "qgc.fgs.channelDesc";
    private static final String EXTRA_NOTIFICATION_TXT = "qgc.fgs.notificationText";
    private static final int    NOTIFICATION_ID        = 1001;

    /**
     * Notification + FGS-type description handed in by the caller. {@code channelId} must be
     * stable per workload so Android doesn't keep recreating channels.
     */
    public record Config(
            int foregroundServiceType,
            String channelId,
            String channelName,
            String channelDescription,
            String notificationText) {
    }

    public static void start(final Context context, final Config config) {
        if (context == null || config == null) {
            return;
        }
        final Context appContext = context.getApplicationContext();
        final Intent intent = new Intent(appContext, QGCForegroundService.class)
                .setAction(ACTION_START)
                .putExtra(EXTRA_FGS_TYPE, config.foregroundServiceType())
                .putExtra(EXTRA_CHANNEL_ID, config.channelId())
                .putExtra(EXTRA_CHANNEL_NAME, config.channelName())
                .putExtra(EXTRA_CHANNEL_DESC, config.channelDescription())
                .putExtra(EXTRA_NOTIFICATION_TXT, config.notificationText());
        appContext.startForegroundService(intent);
    }

    public static void stop(final Context context) {
        if (context == null) {
            return;
        }
        final Context appContext = context.getApplicationContext();
        final Intent intent = new Intent(appContext, QGCForegroundService.class);
        appContext.stopService(intent);
    }

    @Override
    public IBinder onBind(final Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(final Intent intent, final int flags, final int startId) {
        if (intent == null) {
            stopSelf(startId);
            return START_NOT_STICKY;
        }
        final int fgsType = intent.getIntExtra(EXTRA_FGS_TYPE,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_NONE);
        final String channelId   = intent.getStringExtra(EXTRA_CHANNEL_ID);
        final String channelName = intent.getStringExtra(EXTRA_CHANNEL_NAME);
        final String channelDesc = intent.getStringExtra(EXTRA_CHANNEL_DESC);
        final String text        = intent.getStringExtra(EXTRA_NOTIFICATION_TXT);
        if (channelId == null || channelName == null) {
            stopSelf(startId);
            return START_NOT_STICKY;
        }
        createNotificationChannel(channelId, channelName, channelDesc);
        final Notification notification = buildNotification(channelId, text);
        int resolvedFgsType = fgsType;
        if (Build.VERSION.SDK_INT >= 34
                && resolvedFgsType == ServiceInfo.FOREGROUND_SERVICE_TYPE_NONE) {
            android.util.Log.w("QGCForegroundService",
                    "fgsType is NONE on API 34+; defaulting to CONNECTED_DEVICE");
            resolvedFgsType = ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE;
        }
        // startForeground(id, notification, type) requires API 29; minSdk=28 needs the 2-arg
        // overload with the FGS type pulled from the manifest entry.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification, resolvedFgsType);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        stopForeground(STOP_FOREGROUND_REMOVE);
        super.onDestroy();
    }

    private void createNotificationChannel(final String channelId, final String channelName,
                                           final String channelDescription) {
        final NotificationManager manager =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (manager == null || manager.getNotificationChannel(channelId) != null) {
            return;
        }
        final NotificationChannel channel = new NotificationChannel(
                channelId, channelName, NotificationManager.IMPORTANCE_LOW);
        if (channelDescription != null) {
            channel.setDescription(channelDescription);
        }
        manager.createNotificationChannel(channel);
    }

    private Notification buildNotification(final String channelId, final String text) {
        final Intent launchIntent = getPackageManager().getLaunchIntentForPackage(getPackageName());
        final PendingIntent contentIntent = (launchIntent == null) ? null : PendingIntent.getActivity(
                this,
                0,
                launchIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        final CharSequence appLabel = getApplicationInfo().loadLabel(getPackageManager());
        final int icon = (getApplicationInfo().icon != 0)
                ? getApplicationInfo().icon
                : android.R.drawable.sym_def_app_icon;

        final Notification.Builder builder = new Notification.Builder(this, channelId)
                .setSmallIcon(icon)
                .setContentTitle(appLabel)
                .setOngoing(true)
                .setOnlyAlertOnce(true)
                .setCategory(Notification.CATEGORY_SERVICE);
        if (text != null) {
            builder.setContentText(text);
        }
        if (contentIntent != null) {
            builder.setContentIntent(contentIntent);
        }
        return builder.build();
    }
}
