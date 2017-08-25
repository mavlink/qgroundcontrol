package com.yuneec.datapilot;

import android.util.Log;
import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.content.BroadcastReceiver;

public class StartQGCAtBootReceiver extends BroadcastReceiver {

    private static final String TAG = "DataPilot_JNI";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            Log.i(TAG, "DataPilot Auto Start.");
            Intent qgcIntent = new Intent(context, com.yuneec.datapilot.QGCActivity.class);
            qgcIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(qgcIntent);
        }
    }
}
