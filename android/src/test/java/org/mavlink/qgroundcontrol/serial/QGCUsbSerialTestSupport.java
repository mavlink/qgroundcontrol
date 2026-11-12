package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertTrue;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.FakeUsbDeviceConnection;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

/** The fakes and helpers that the tests of the USB serial classes share. */
final class QGCUsbSerialTestSupport {
    static final String DEVICE_NAME = "/dev/bus/usb/001/002";

    @FunctionalInterface
    interface PortMethod {
        Object invoke(Object[] args) throws Exception;
    }

    private record FakeDriver(UsbDevice device, List<UsbSerialPort> ports) implements UsbSerialDriver {
        @Override
        public UsbDevice getDevice() {
            return device;
        }

        @Override
        public List<UsbSerialPort> getPorts() {
            return ports;
        }
    }

    /** A driver of the given ports as they are, for a test whose ports answer every call themselves. */
    static UsbSerialDriver driver(final UsbDevice device, final UsbSerialPort... ports) {
        return new FakeDriver(device, List.of(ports));
    }

    static final class FakeUsbHost implements QGCUsbSerialHost {
        final List<UsbSerialDriver> drivers = new ArrayList<>();
        final List<UsbDevice> permissionRequests = new ArrayList<>();
        final List<FakeUsbDeviceConnection> connections = new ArrayList<>();
        boolean grantsPermission = true;
        boolean opensConnections = true;
        int permissionChecks;
        Exception driverOpenFailure;
        Exception driverCloseFailure;
        /** What the search for drivers throws, as the library's prober does for a driver it cannot create. */
        RuntimeException findDriversFailure;
        /** Called with each device that a driver of its own is asked for, before the host answers. */
        Consumer<UsbDevice> onProbe = device -> {};

        /** Attaches a single-port device; the result tells whether its driver port is open. */
        AtomicBoolean attach(final UsbDevice device) {
            return attach(device, 1).get(0);
        }

        /** Attaches a device; the result tells, for each of its ports, whether the driver port is open. */
        List<AtomicBoolean> attach(final UsbDevice device, final int portCount) {
            final UsbSerialPort[] driverPorts = new UsbSerialPort[portCount];
            // Ports with no methods of their own: a test can only list, open and close them.
            Arrays.fill(driverPorts, port(Map.of()));
            return attach(device, driverPorts);
        }

        /**
         * Attaches a device with a port for each of the given ones, which a test can then open as C++ does. The
         * host answers what it takes to list, open and close a port: which device it is on and whether it is open.
         * A given port is asked everything else, so it needs only the methods that its test uses on the open
         * port. The result tells, for each port, whether the driver port is open.
         */
        List<AtomicBoolean> attach(final UsbDevice device, final UsbSerialPort... driverPorts) {
            final List<AtomicBoolean> openFlags = new ArrayList<>();
            final List<UsbSerialPort> ports = new ArrayList<>();
            for (int i = 0; i < driverPorts.length; i++) {
                final AtomicBoolean open = new AtomicBoolean();
                openFlags.add(open);
                ports.add(port(Map.of(
                        "getDevice", args -> device,
                        "isOpen", args -> open.get(),
                        "open", args -> {
                            if (driverOpenFailure != null) {
                                throw driverOpenFailure;
                            }
                            open.set(true);
                            return null;
                        },
                        "close", args -> {
                            open.set(false);
                            if (driverCloseFailure != null) {
                                throw driverCloseFailure;
                            }
                            return null;
                        }), driverPorts[i]));
            }
            drivers.add(new FakeDriver(device, ports));
            return openFlags;
        }

        @Override
        public List<UsbSerialDriver> findDrivers() {
            if (findDriversFailure != null) {
                throw findDriversFailure;
            }
            return new ArrayList<>(drivers);
        }

        /** Returns the driver that the device is attached with, and makes none of its own. */
        @Override
        public UsbSerialDriver probeDriver(final UsbDevice device) {
            onProbe.accept(device);
            for (final UsbSerialDriver driver : drivers) {
                if (driver.getDevice() == device) {
                    return driver;
                }
            }
            return null;
        }

        @Override
        public boolean hasPermission(final UsbDevice device) {
            permissionChecks++;
            return grantsPermission && !withoutPermission.contains(device);
        }

        @Override
        public void requestPermission(final UsbDevice device) {
            permissionRequests.add(device);
        }

        @Override
        public UsbDeviceConnection openDevice(final UsbDevice device) {
            if (!opensConnections) {
                return null;
            }
            connections.add(new FakeUsbDeviceConnection());
            return connections.get(connections.size() - 1);
        }

        /** The devices that have no permission while {@code grantsPermission} gives it to the others. */
        final Set<UsbDevice> withoutPermission = new HashSet<>();
        /** Whether QGC's window has the focus. */
        boolean windowFocused = true;

        @Override
        public boolean canAskUser() {
            return windowFocused;
        }
    }

    static final class RecordingCallbacks implements QGCUsbSerialCallbacks {
        // The I/O threads report here too.
        final List<String> events = new CopyOnWriteArrayList<>();
        /** What C++ answers to input: whether it has room for more. */
        volatile boolean hasRoom = true;

        @Override
        public boolean newData(final long token, final byte[] data, final int length) {
            events.add("data " + token + " " + length);
            return hasRoom;
        }

        @Override
        public void bytesWritten(final long token, final int count) {
            events.add("written " + token + " " + count);
        }

        // The reason is its number in QGCUsbSerialResourceError: 1 detached, 2 read failed, 3 write failed.
        @Override
        public void resourceError(final long token, final int reason, final String detail) {
            events.add("resourceError " + token + " " + reason + " [" + detail + "]");
        }

        @Override
        public void devicesChanged() {
            events.add("devicesChanged");
        }
    }

    private QGCUsbSerialTestSupport() {}

    /** The fields of each port, in the order the constructor takes them. */
    static List<List<Object>> fieldsOf(final QGCUsbSerialPortInfo... ports) {
        final List<List<Object>> fields = new ArrayList<>();
        for (final QGCUsbSerialPortInfo port : ports) {
            fields.add(List.of(port.name, port.stableName, port.productName, port.manufacturerName,
                               port.serialNumber, port.vendorId, port.productId));
        }
        return fields;
    }

    static FakeUsbDevice twoPortAdapter(final int deviceId, final String deviceName) {
        final FakeUsbDevice adapter = new FakeUsbDevice(deviceId, deviceName);
        adapter.vendorId = 0x0403;
        adapter.productId = 0x6010;
        adapter.serialNumber = "A1";
        return adapter;
    }

    /**
     * A port that a read thread can run on. Each read waits for a permit and then returns no data, as on a line
     * where nothing arrives.
     */
    static UsbSerialPort portOnAnIdleLine(final Semaphore readsToEnd) {
        return port(Map.of(
                "isOpen", args -> true,
                "close", args -> null,
                "setReadQueue", args -> null,
                "read", args -> {
                    readsToEnd.tryAcquire(5, TimeUnit.SECONDS);
                    return 0;
                }));
    }

    /** The port's write thread, or a thread that has ended if the port's has. */
    static Thread writeThread(final QGCUsbSerialPort port) {
        return ioThread("QGCUsbSerialWrite-" + port.id);
    }

    /** The port's read thread, or a thread that has ended if the port's has. */
    static Thread readThread(final QGCUsbSerialPort port) {
        return ioThread("QGCUsbSerialRead-" + port.id);
    }

    private static Thread ioThread(final String name) {
        for (final Thread thread : Thread.getAllStackTraces().keySet()) {
            if (thread.getName().equals(name)) {
                return thread;
            }
        }
        return new Thread();
    }

    static void awaitTrue(final BooleanSupplier condition) throws InterruptedException {
        final long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5);
        while (!condition.getAsBoolean()) {
            assertTrue("Timed out", System.nanoTime() < deadline);
            Thread.sleep(1);
        }
    }

    /** A driver port with the given methods, by name. A call of any other method fails the test. */
    static UsbSerialPort port(final Map<String, PortMethod> methods) {
        return (UsbSerialPort) Proxy.newProxyInstance(UsbSerialPort.class.getClassLoader(),
                new Class<?>[] {UsbSerialPort.class}, (proxy, method, args) -> {
                    final PortMethod handler = methods.get(method.getName());
                    if (handler == null) {
                        throw new AssertionError("Unexpected call: " + method.getName());
                    }
                    return handler.invoke(args);
                });
    }

    /** A driver port with the given methods, by name. It passes a call of any other method on to {@code other}. */
    private static UsbSerialPort port(final Map<String, PortMethod> methods, final UsbSerialPort other) {
        return (UsbSerialPort) Proxy.newProxyInstance(UsbSerialPort.class.getClassLoader(),
                new Class<?>[] {UsbSerialPort.class}, (proxy, method, args) -> {
                    final PortMethod handler = methods.get(method.getName());
                    if (handler != null) {
                        return handler.invoke(args);
                    }
                    try {
                        return method.invoke(other, args);
                    } catch (final InvocationTargetException e) {
                        // What the other port threw, as its caller would have got it.
                        throw e.getCause();
                    }
                });
    }
}
