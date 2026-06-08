package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ComponentResolutionInstrumentedTest {

    @Test
    public void foregroundServiceClassResolves() throws ClassNotFoundException {
        final ClassLoader loader = getClass().getClassLoader();
        final Class<?> service =
                Class.forName("org.mavlink.qgroundcontrol.QGCForegroundService", false, loader);
        assertTrue(android.app.Service.class.isAssignableFrom(service));
    }

    @Test
    public void foregroundServiceDeclaredInManifest() throws PackageManager.NameNotFoundException {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final ComponentName component =
                new ComponentName(context, "org.mavlink.qgroundcontrol.QGCForegroundService");
        final ServiceInfo info =
                context.getPackageManager().getServiceInfo(component, 0);
        assertNotNull(info);
    }

    @Test
    public void mainActivityClassResolves() throws ClassNotFoundException {
        final ClassLoader loader = getClass().getClassLoader();
        assertNotNull(Class.forName("org.mavlink.qgroundcontrol.QGCActivity", false, loader));
    }
}
