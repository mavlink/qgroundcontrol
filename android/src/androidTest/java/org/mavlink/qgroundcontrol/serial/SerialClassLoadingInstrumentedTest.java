package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SerialClassLoadingInstrumentedTest {

    @Test
    public void targetPackageIdentifierMatches() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assertEquals("org.mavlink.qgroundcontrol", context.getPackageName());
    }

    @Test
    public void serialClassesLoadOnArtRuntime() throws ClassNotFoundException {
        final ClassLoader loader = getClass().getClassLoader();
        assertNotNull(loader);

        for (final String name : new String[] {
                "org.mavlink.qgroundcontrol.serial.UsbSerialEnumerator",
                "org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager",
                "org.mavlink.qgroundcontrol.serial.QGCSerialPort",
                "org.mavlink.qgroundcontrol.serial.UsbAttachDetachReceiver",
                "org.mavlink.qgroundcontrol.serial.UsbPermissionManager" }) {
            assertNotNull(name, Class.forName(name, false, loader));
        }
    }
}
