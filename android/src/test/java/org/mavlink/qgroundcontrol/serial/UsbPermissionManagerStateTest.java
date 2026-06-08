package org.mavlink.qgroundcontrol.serial;

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
public class UsbPermissionManagerStateTest {

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

    private void deliverMalformedResult() {
        context.sendBroadcast(new Intent(ACTION_USB_PERMISSION).setPackage(context.getPackageName()));
        Shadows.shadowOf(RuntimeEnvironment.getApplication().getMainLooper()).idle();
    }

    @Before
    public void setUp() {
        context = RuntimeEnvironment.getApplication();
        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        shadowUsbManager = Shadow.extract(usbManager);
        manager = new UsbPermissionManager(granted::add, denied::add);
    }

    @Test
    public void deniedThenCleared_isPermissionDeniedFlipsBack() {
        manager.register(context);
        final UsbDevice dev = device("/dev/bus/usb/002/001");
        shadowUsbManager.addOrUpdateUsbDevice(dev, false);
        manager.request(usbManager, dev);
        deliverMalformedResult();

        assertTrue(manager.isPermissionDenied("/dev/bus/usb/002/001"));

        manager.clearPermission("/dev/bus/usb/002/001");
        assertFalse(manager.isPermissionDenied("/dev/bus/usb/002/001"));
    }

    @Test
    public void grantedDevice_isNotReportedDenied() {
        manager.register(context);
        final UsbDevice dev = device("/dev/bus/usb/002/002");
        shadowUsbManager.addOrUpdateUsbDevice(dev, true);
        manager.request(usbManager, dev);
        deliverMalformedResult();

        assertFalse(manager.isPermissionDenied("/dev/bus/usb/002/002"));
    }

    @Test
    public void unknownDevice_isNotReportedDenied() {
        assertFalse(manager.isPermissionDenied("/dev/bus/usb/002/099"));
    }

    @Test
    public void requestBeforeRegister_noOpsAndLeavesNoPendingState() {
        final UsbDevice dev = device("/dev/bus/usb/002/003");
        shadowUsbManager.addOrUpdateUsbDevice(dev, false);

        manager.request(usbManager, dev);

        assertFalse(manager.isPermissionDenied("/dev/bus/usb/002/003"));

        manager.register(context);
        deliverMalformedResult();
        assertTrue(granted.isEmpty());
        assertTrue(denied.isEmpty());
    }

    @Test
    public void unregisterClearsState_lateBroadcastResolvesNothing() {
        manager.register(context);
        final UsbDevice dev = device("/dev/bus/usb/002/004");
        shadowUsbManager.addOrUpdateUsbDevice(dev, false);
        manager.request(usbManager, dev);

        manager.unregister();

        assertFalse(manager.isPermissionDenied("/dev/bus/usb/002/004"));

        deliverMalformedResult();
        assertTrue(granted.isEmpty());
        assertTrue(denied.isEmpty());
    }
}
