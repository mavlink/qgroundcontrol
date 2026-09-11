package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertArrayEquals;

import java.io.IOException;
import java.lang.reflect.Proxy;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.SerialTimeoutException;
import static org.junit.Assert.assertNotEquals;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class QGCUsbSerialManagerTest {

    @Before
    public void setUp() {
        QGCUsbSerialManager.resetResourceMappingsForTesting();
    }

    @After
    public void tearDown() {
        QGCUsbSerialManager.resetResourceMappingsForTesting();
    }

    @Test
    public void parseDevicePortSpec_withoutSuffix_defaultsToPortZero() {
        final String deviceName = "/dev/bus/usb/001/002";
        assertEquals(deviceName, QGCUsbSerialManager.getBaseDeviceNameForTesting(deviceName));
        assertEquals(0, QGCUsbSerialManager.getPortIndexForTesting(deviceName));
    }

    @Test
    public void parseDevicePortSpec_withSuffix_extractsPortIndex() {
        assertEquals("/dev/bus/usb/001/002", QGCUsbSerialManager.getBaseDeviceNameForTesting("/dev/bus/usb/001/002#p3"));
        assertEquals(3, QGCUsbSerialManager.getPortIndexForTesting("/dev/bus/usb/001/002#p3"));
    }

    @Test
    public void parseDevicePortSpec_invalidSuffix_fallsBackToOriginalNameAndPortZero() {
        final String malformed = "/dev/bus/usb/001/002#pabc";
        assertEquals(malformed, QGCUsbSerialManager.getBaseDeviceNameForTesting(malformed));
        assertEquals(0, QGCUsbSerialManager.getPortIndexForTesting(malformed));
    }

    @Test
    public void resourceIdMapping_reusesSameAddressAndSeparatesPorts() {
        final int first = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 0);
        final int second = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 0);
        final int otherPort = QGCUsbSerialManager.getOrCreateResourceIdForTesting(42, 1);

        assertEquals(first, second);
        assertNotEquals(first, otherPort);
    }

    @Test
    public void resourceIdMapping_removedAddressGetsNewId() {
        final int first = QGCUsbSerialManager.getOrCreateResourceIdForTesting(7, 2);
        QGCUsbSerialManager.removeResourceMappingForTesting(first);
        final int next = QGCUsbSerialManager.getOrCreateResourceIdForTesting(7, 2);

        assertNotEquals(first, next);
    }

    @Test
    public void writeResult_preservesTimeoutPrefixAndUnknownSuffix() {
        final UsbSerialPort port = (UsbSerialPort) Proxy.newProxyInstance(
                UsbSerialPort.class.getClassLoader(), new Class<?>[] {UsbSerialPort.class},
                (proxy, method, args) -> { throw new SerialTimeoutException("partial", 3); });
        assertArrayEquals(new int[] {1, 3, 5},
                QGCUsbSerialManager.writeResultForPort(port, new byte[8], 8, 25));
    }

    @Test
    public void writeResult_retainsUncertaintyForIoFailure() {
        final UsbSerialPort port = (UsbSerialPort) Proxy.newProxyInstance(
                UsbSerialPort.class.getClassLoader(), new Class<?>[] {UsbSerialPort.class},
                (proxy, method, args) -> { throw new IOException("disconnected"); });
        assertArrayEquals(new int[] {2, 0, 8},
                QGCUsbSerialManager.writeResultForPort(port, new byte[8], 8, 25));
    }

    @Test
    public void writeResult_forwardsFiniteInitialTimeout() {
        final int[] timeout = {0};
        final UsbSerialPort port = (UsbSerialPort) Proxy.newProxyInstance(
                UsbSerialPort.class.getClassLoader(), new Class<?>[] {UsbSerialPort.class},
                (proxy, method, args) -> { timeout[0] = (int) args[2]; return null; });
        assertArrayEquals(new int[] {0, 8, 0},
                QGCUsbSerialManager.writeResultForPort(port, new byte[8], 8, 25));
        assertEquals(25, timeout[0]);
        assertArrayEquals(new int[] {2, 0, 0},
                QGCUsbSerialManager.writeResultForPort(port, new byte[8], 8, 0));
        assertEquals(25, timeout[0]);
    }
    @Test
    public void writeResult_missingDeviceHasNoUncertainBytes() {
        assertArrayEquals(new int[] {2, 0, 0},
                QGCUsbSerialManager.writeResultForPort(null, new byte[8], 8, 25));
    }

    @Test
    public void writeResult_completedPrefixOnTimeoutHasNoUnknownSuffix() {
        final UsbSerialPort port = (UsbSerialPort) Proxy.newProxyInstance(
                UsbSerialPort.class.getClassLoader(), new Class<?>[] {UsbSerialPort.class},
                (proxy, method, args) -> { throw new SerialTimeoutException("late", 8); });
        assertArrayEquals(new int[] {1, 8, 0},
                QGCUsbSerialManager.writeResultForPort(port, new byte[8], 8, 25));
    }
}
