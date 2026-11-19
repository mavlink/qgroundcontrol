package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;

import android.content.Intent;
import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Parcelable;

import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialAndroidHost.UsbBroadcast;

public class QGCUsbSerialAndroidHostTest {
    /**
     * An intent for JVM unit tests, where Android's own holds nothing. Like Android's, it answers with the
     * default for an extra that it lacks.
     */
    private static final class FakeIntent extends Intent {
        private final String _action;
        private final UsbDevice _device;
        private final Boolean _permissionGranted;

        FakeIntent(final String action, final UsbDevice device, final Boolean permissionGranted) {
            _action = action;
            _device = device;
            _permissionGranted = permissionGranted;
        }

        @Override
        public String getAction() {
            return _action;
        }

        @Override
        public boolean getBooleanExtra(final String name, final boolean defaultValue) {
            final boolean present = UsbManager.EXTRA_PERMISSION_GRANTED.equals(name) && _permissionGranted != null;
            return present ? _permissionGranted : defaultValue;
        }

        @Override
        @SuppressWarnings({"deprecation", "unchecked"})
        public <T extends Parcelable> T getParcelableExtra(final String name) {
            return UsbManager.EXTRA_DEVICE.equals(name) ? (T) _device : null;
        }

        @Override
        public <T> T getParcelableExtra(final String name, final Class<T> type) {
            return UsbManager.EXTRA_DEVICE.equals(name) ? type.cast(_device) : null;
        }
    }

    private final UsbDevice _device = new FakeUsbDevice(1002, DEVICE_NAME);

    @Test
    public void decode_namesTheEventAndTheDeviceOfEachBroadcast() {
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.ATTACHED, _device),
                     decode(UsbManager.ACTION_USB_DEVICE_ATTACHED, _device, null));
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.DETACHED, _device),
                     decode(UsbManager.ACTION_USB_DEVICE_DETACHED, _device, null));
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.PERMISSION_GRANTED, _device),
                     decode(QGCUsbSerialAndroidHost.ACTION_USB_PERMISSION, _device, true));
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.PERMISSION_DENIED, _device),
                     decode(QGCUsbSerialAndroidHost.ACTION_USB_PERMISSION, _device, false));
    }

    @Test
    public void decode_ofAPermissionAnswerThatDoesNotSayWhatTheUserChose_isARefusal() {
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.PERMISSION_DENIED, _device),
                     decode(QGCUsbSerialAndroidHost.ACTION_USB_PERMISSION, _device, null));
    }

    @Test
    public void decode_ofABroadcastThatNamesNoDevice_hasNoDevice() {
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.DETACHED, null),
                     decode(UsbManager.ACTION_USB_DEVICE_DETACHED, null, null));
        assertEquals(new UsbBroadcast(QGCUsbSerialEvent.PERMISSION_GRANTED, null),
                     decode(QGCUsbSerialAndroidHost.ACTION_USB_PERMISSION, null, true));
    }

    @Test
    public void decode_ofAnActionThatTheReceiverDoesNotRegisterFor_isNothing() {
        assertNull(decode(UsbManager.ACTION_USB_ACCESSORY_ATTACHED, _device, true));
        assertNull(decode(null, _device, true));
    }

    private static UsbBroadcast decode(final String action, final UsbDevice device, final Boolean permissionGranted) {
        return QGCUsbSerialAndroidHost.decode(new FakeIntent(action, device, permissionGranted));
    }
}
