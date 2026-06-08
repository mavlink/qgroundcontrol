package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertNotNull;

import android.content.Context;
import android.hardware.usb.UsbManager;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UsbManagerEnumerationInstrumentedTest {

    @Test
    public void usbServiceResolves() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        assertNotNull(usbManager);
    }
}
