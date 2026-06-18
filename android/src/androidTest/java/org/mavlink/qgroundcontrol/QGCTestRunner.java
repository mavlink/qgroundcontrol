package org.mavlink.qgroundcontrol;

import android.app.Application;
import android.content.Context;

import androidx.test.runner.AndroidJUnitRunner;

// Instrument against a plain Application instead of the target app's QtApplication, so QGC's
// app-level init stays out of these class-loading smoke tests.
public final class QGCTestRunner extends AndroidJUnitRunner {
    @Override
    public Application newApplication(ClassLoader cl, String className, Context context)
            throws InstantiationException, IllegalAccessException, ClassNotFoundException {
        return super.newApplication(cl, Application.class.getName(), context);
    }
}
