package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
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
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.ShadowUsbManager;
import org.robolectric.util.ReflectionHelpers;

import java.util.ArrayList;
import java.util.List;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class)
public class UsbPermissionManagerMalformedBroadcastTest {

    private static final String ACTION_USB_PERMISSION =
            "org.mavlink.qgroundcontrol.action.USB_PERMISSION";

    private Context context;
    private UsbManager usbManager;
    private ShadowUsbManager shadowUsbManager;
    private final List<UsbDevice> granted = new ArrayList<>();
    private final List<UsbDevice> denied = new ArrayList<>();
    private UsbPermissionManager manager;

    private static UsbDevice device(final String name) {
        final UsbDevice device = ReflectionHelpers.callConstructor(UsbDevice.class);
        ReflectionHelpers.setField(device, "mName", name);
        return device;
    }

    private Intent malformedPermissionResult() {
        return new Intent(ACTION_USB_PERMISSION).setPackage(context.getPackageName());
    }

    @Before
    public void setUp() {
        context = RuntimeEnvironment.getApplication();
        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        shadowUsbManager = Shadow.extract(usbManager);
        manager = new UsbPermissionManager(granted::add, denied::add);
        manager.register(context);
    }

    @Test
    public void malformedBroadcast_zeroPending_resolvesNothing() {
        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertTrue(granted.isEmpty());
        assertTrue(denied.isEmpty());
    }

    @Test
    public void malformedBroadcast_singlePendingGranted_resolvesThatDevice() {
        final UsbDevice dev = device("/dev/bus/usb/001/002");
        shadowUsbManager.addOrUpdateUsbDevice(dev, true);
        manager.request(usbManager, dev);

        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertEquals(1, granted.size());
        assertEquals("/dev/bus/usb/001/002", granted.get(0).getDeviceName());
        assertTrue(denied.isEmpty());
        assertFalse(manager.isPermissionDenied("/dev/bus/usb/001/002"));
    }

    @Test
    public void malformedBroadcast_singlePendingNoSystemGrant_resolvesAsDenied() {
        final UsbDevice dev = device("/dev/bus/usb/001/003");
        shadowUsbManager.addOrUpdateUsbDevice(dev, false);
        manager.request(usbManager, dev);

        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertEquals(1, denied.size());
        assertEquals("/dev/bus/usb/001/003", denied.get(0).getDeviceName());
        assertTrue(granted.isEmpty());
        assertTrue(manager.isPermissionDenied("/dev/bus/usb/001/003"));
    }

    @Test
    public void detachClearsPermissionMidDialog_malformedGrantStillResolves() {
        final UsbDevice dev = device("/dev/bus/usb/001/006");
        shadowUsbManager.addOrUpdateUsbDevice(dev, true);
        manager.request(usbManager, dev);

        manager.clearPermission(dev.getDeviceName());

        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertEquals(1, granted.size());
        assertEquals("/dev/bus/usb/001/006", granted.get(0).getDeviceName());
        assertTrue(denied.isEmpty());
    }

    @Test
    public void malformedBroadcast_staleDetachedEntry_resolvesLiveGrant() {
        final UsbDevice stale = device("/dev/bus/usb/001/007");
        final UsbDevice live = device("/dev/bus/usb/001/008");
        shadowUsbManager.addOrUpdateUsbDevice(stale, false);
        shadowUsbManager.addOrUpdateUsbDevice(live, true);
        manager.request(usbManager, stale);
        manager.clearPermission(stale.getDeviceName());
        manager.request(usbManager, live);

        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertEquals(1, granted.size());
        assertEquals("/dev/bus/usb/001/008", granted.get(0).getDeviceName());
        assertTrue(denied.isEmpty());
    }

    @Test
    public void malformedBroadcast_multiplePending_resolvesNothing() {
        final UsbDevice first = device("/dev/bus/usb/001/004");
        final UsbDevice second = device("/dev/bus/usb/001/005");
        shadowUsbManager.addOrUpdateUsbDevice(first, true);
        shadowUsbManager.addOrUpdateUsbDevice(second, true);
        manager.request(usbManager, first);
        manager.request(usbManager, second);

        context.sendBroadcast(malformedPermissionResult());
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();

        assertTrue(granted.isEmpty());
        assertTrue(denied.isEmpty());
    }
}
