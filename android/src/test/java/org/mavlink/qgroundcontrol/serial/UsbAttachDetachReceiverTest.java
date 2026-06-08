package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import java.util.ArrayList;
import java.util.List;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class)
public class UsbAttachDetachReceiverTest {

    private Context context;
    private final List<UsbDevice> attached = new ArrayList<>();
    private final List<UsbDevice> detached = new ArrayList<>();
    private UsbAttachDetachReceiver receiver;

    private static UsbDevice device(final String name) {
        final UsbDevice device = ReflectionHelpers.callConstructor(UsbDevice.class);
        ReflectionHelpers.setField(device, "mName", name);
        return device;
    }

    private void deliver(final Intent intent) {
        context.sendBroadcast(intent);
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();
    }

    @Before
    public void setUp() {
        context = RuntimeEnvironment.getApplication();
        receiver = new UsbAttachDetachReceiver(attached::add, detached::add);
        receiver.register(context);
    }

    @Test
    public void attachAction_invokesOnAttachedOnly() {
        final UsbDevice dev = device("/dev/bus/usb/001/010");
        deliver(new Intent(UsbManager.ACTION_USB_DEVICE_ATTACHED)
                .putExtra(UsbManager.EXTRA_DEVICE, dev));

        assertEquals(1, attached.size());
        assertSame(dev, attached.get(0));
        assertTrue(detached.isEmpty());
    }

    @Test
    public void detachAction_invokesOnDetachedOnly() {
        final UsbDevice dev = device("/dev/bus/usb/001/011");
        deliver(new Intent(UsbManager.ACTION_USB_DEVICE_DETACHED)
                .putExtra(UsbManager.EXTRA_DEVICE, dev));

        assertEquals(1, detached.size());
        assertSame(dev, detached.get(0));
        assertTrue(attached.isEmpty());
    }

    @Test
    public void attachAction_nullDevice_isIgnored() {
        deliver(new Intent(UsbManager.ACTION_USB_DEVICE_ATTACHED));

        assertTrue(attached.isEmpty());
        assertTrue(detached.isEmpty());
    }

    @Test
    public void unrelatedAction_withDevice_isIgnored() {
        final UsbDevice dev = device("/dev/bus/usb/001/012");
        deliver(new Intent("org.mavlink.qgroundcontrol.action.UNRELATED")
                .setPackage(context.getPackageName())
                .putExtra(UsbManager.EXTRA_DEVICE, dev));

        assertTrue(attached.isEmpty());
        assertTrue(detached.isEmpty());
    }
}
