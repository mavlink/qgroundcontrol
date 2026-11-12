package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.awaitTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.driver;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.fieldsOf;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.port;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.twoPortAdapter;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.FakeUsbHost;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.RecordingCallbacks;

public class QGCUsbSerialDevicesTest {
    @Test
    public void open_refusesASecondOpenerUntilThePortIsClosed() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertEquals(QGCUsbSerialResult.ALREADY_OPEN, devices.open(DEVICE_NAME, 8));

        assertEquals(QGCUsbSerialResult.OK, devices.close(7));
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 8));
    }

    @Test
    public void open_ofADeviceKnownToHavePermission_doesNotAskAndroidForItAgain() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());
        devices.availablePorts();
        final int permissionChecks = usbHost.permissionChecks;

        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertEquals(permissionChecks, usbHost.permissionChecks);
    }

    @Test
    public void permissionQueries_askAndroidOnlyForWhatIsNotKnownYet() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        // Once for each query, for as long as the answer is no.
        int permissionChecks = usbHost.permissionChecks;
        assertFalse(devices.hasPermission(DEVICE_NAME));
        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals(permissionChecks + 2, usbHost.permissionChecks);

        // A permission lasts until its device detaches.
        usbHost.grantsPermission = true;
        assertTrue(devices.hasPermission(DEVICE_NAME));
        permissionChecks = usbHost.permissionChecks;
        assertTrue(devices.hasPermission(DEVICE_NAME));
        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals(permissionChecks, usbHost.permissionChecks);
    }

    @Test
    public void open_reportsWhyItFailed() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("/dev/bus/usb/001/099", 7));

        assertEquals(QGCUsbSerialResult.PERMISSION_DENIED, devices.open(DEVICE_NAME, 7));
        assertTrue(usbHost.connections.isEmpty());

        usbHost.grantsPermission = true;
        usbHost.driverOpenFailure = new IOException("Driver rejected the device");
        assertEquals(QGCUsbSerialResult.FAILED, devices.open(DEVICE_NAME, 7));
        assertTrue(usbHost.connections.get(0).closed);

        // Not an error the driver declares, but it must not leave the port registered either.
        usbHost.driverOpenFailure = new IllegalStateException("Unexpected descriptor");
        assertEquals(QGCUsbSerialResult.FAILED, devices.open(DEVICE_NAME, 7));
        assertTrue(usbHost.connections.get(1).closed);

        usbHost.driverOpenFailure = null;
        // A port needs a handle, and one that names no other port.
        assertEquals(QGCUsbSerialResult.FAILED, devices.open(DEVICE_NAME, 0));
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertEquals(QGCUsbSerialResult.FAILED, devices.open(DEVICE_NAME, 7));
    }

    @Test
    public void stableName_stillOpensThePortAfterAReplug() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final FakeUsbDevice adapter = twoPortAdapter(1002, DEVICE_NAME);
        usbHost.attach(adapter, 2);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());
        assertEquals(fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME + "#p0", "usb:0403:6010:A1#p0", "", "", "A1",
                                                       0x0403, 0x6010),
                              new QGCUsbSerialPortInfo(DEVICE_NAME + "#p1", "usb:0403:6010:A1#p1", "", "", "A1",
                                                       0x0403, 0x6010)),
                     fieldsOf(devices.availablePorts()));

        // Replugged, Android gives the adapter a new device ID and name.
        usbHost.drivers.clear();
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, adapter);
        final FakeUsbDevice replugged = twoPortAdapter(1003, "/dev/bus/usb/001/003");
        final List<AtomicBoolean> driverPortsOpen = usbHost.attach(replugged, 2);
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, replugged);

        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open(DEVICE_NAME + "#p1", 7));
        assertEquals(QGCUsbSerialResult.OK, devices.open("usb:0403:6010:A1#p1", 7));
        assertFalse(driverPortsOpen.get(0).get());
        assertTrue(driverPortsOpen.get(1).get());
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6010:B2", 8));
    }

    @Test
    public void stableName_ofASerialNumberThatEndsLikeAPortSuffix_opensThePort() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final FakeUsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        board.serialNumber = "AB#p1";
        final AtomicBoolean driverPortOpen = usbHost.attach(board);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        // The only port of a device is named without a suffix, so nothing else in the name may read as one.
        final String stableName = devices.availablePorts()[0].stableName;
        assertEquals(QGCUsbSerialResult.OK, devices.open(stableName, 7));
        assertTrue(driverPortOpen.get());
    }

    @Test
    public void ports_ofOneOrSeveralDevicesCanBeOpenTogether() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final List<AtomicBoolean> adapterPortsOpen = usbHost.attach(twoPortAdapter(1002, DEVICE_NAME), 2);
        final AtomicBoolean boardPortOpen = usbHost.attach(new FakeUsbDevice(1003, "/dev/bus/usb/001/003"));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME + "#p0", 7));
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME + "#p1", 8));
        assertEquals(QGCUsbSerialResult.OK, devices.open("/dev/bus/usb/001/003", 9));
        assertTrue(adapterPortsOpen.get(0).get() && adapterPortsOpen.get(1).get() && boardPortOpen.get());
        assertEquals(8, devices.portHandle(DEVICE_NAME + "#p1"));

        assertEquals(QGCUsbSerialResult.OK, devices.close(7));
        assertFalse(adapterPortsOpen.get(0).get());
        assertTrue(adapterPortsOpen.get(1).get() && boardPortOpen.get());
        assertEquals(8, devices.portHandle(DEVICE_NAME + "#p1"));
        assertEquals(0, devices.portHandle(DEVICE_NAME + "#p0"));
    }

    @Test
    public void portIndex_outsideTheDevicesPorts_namesNoPort() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(twoPortAdapter(1002, DEVICE_NAME), 2);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        assertTrue(devices.isPortNameValid(DEVICE_NAME + "#p1"));
        assertFalse(devices.isPortNameValid(DEVICE_NAME + "#p2"));
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open(DEVICE_NAME + "#p2", 7));
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6010:A1#p2", 7));
        assertEquals(0, devices.portHandle(DEVICE_NAME + "#p2"));
    }

    @Test
    public void detach_closesThePortAndTellsCpp() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final UsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean driverPortOpen = usbHost.attach(device);
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertTrue(driverPortOpen.get());

        usbHost.drivers.clear();
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, device);

        assertFalse(driverPortOpen.get());
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), callbacks.events);
        assertNull(devices.port(7));
    }

    @Test
    public void detach_forgetsTheDeviceEvenWhenItsIdIsAlreadyReused() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final FakeUsbDevice adapter = new FakeUsbDevice(1002, DEVICE_NAME);
        adapter.productName = "FT232R";
        usbHost.attach(adapter);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        final FakeUsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        board.productName = "CubeOrange+";
        usbHost.drivers.clear();
        usbHost.attach(board);
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, adapter);

        assertEquals(fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME, "usb:0000:0000:", "CubeOrange+", "", "", 0, 0)),
                     fieldsOf(devices.availablePorts()));
    }

    @Test
    public void usbEvent_duringTheSyncAtStartup_keepsTheDeviceItBrought() throws Exception {
        final FakeUsbHost attached = new FakeUsbHost();
        final CountDownLatch startupHasRead = new CountDownLatch(1);
        final CountDownLatch startupMayGoOn = new CountDownLatch(1);
        final QGCUsbSerialHost usbHost = new QGCUsbSerialHost() {
            @Override
            public List<UsbSerialDriver> findDrivers() {
                final List<UsbSerialDriver> drivers = attached.findDrivers();
                // The sync at startup has read the attached devices and not yet acted on what it read.
                if (startupHasRead.getCount() > 0) {
                    startupHasRead.countDown();
                    try {
                        startupMayGoOn.await(5, TimeUnit.SECONDS);
                    } catch (final InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
                return drivers;
            }

            @Override
            public UsbSerialDriver probeDriver(final UsbDevice device) {
                return attached.probeDriver(device);
            }

            @Override
            public boolean hasPermission(final UsbDevice device) {
                return attached.hasPermission(device);
            }

            @Override
            public void requestPermission(final UsbDevice device) {
                attached.requestPermission(device);
            }

            @Override
            public UsbDeviceConnection openDevice(final UsbDevice device) {
                return attached.openDevice(device);
            }

            @Override
            public boolean canAskUser() {
                return attached.canAskUser();
            }
        };
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = new QGCUsbSerialDevices(callbacks);
        final Thread startup = new Thread(() -> devices.start(usbHost, callbacks));
        startup.start();
        assertTrue(startupHasRead.await(5, TimeUnit.SECONDS));

        // The app starts on one thread and USB events arrive on another.
        final UsbDevice adapter = new FakeUsbDevice(1002, DEVICE_NAME);
        attached.attach(adapter);
        final Thread event = new Thread(() -> devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, adapter));
        event.start();
        awaitTrue(() -> event.getState() == Thread.State.BLOCKED || !event.isAlive());
        startupMayGoOn.countDown();
        startup.join(5000);
        event.join(5000);

        // What the startup read is older than the event, and must not undo it.
        assertEquals(1, devices.availablePorts().length);
    }

    @Test
    public void availablePorts_asksAndroidAboutAPermissionWithoutHoldingTheLock() {
        final FakeUsbHost attached = new FakeUsbHost();
        attached.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        attached.grantsPermission = false;
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = new QGCUsbSerialDevices(callbacks);
        final AtomicBoolean listing = new AtomicBoolean();
        final AtomicBoolean lockWasFree = new AtomicBoolean();
        final QGCUsbSerialHost usbHost = new QGCUsbSerialHost() {
            @Override
            public List<UsbSerialDriver> findDrivers() {
                return attached.findDrivers();
            }

            @Override
            public UsbSerialDriver probeDriver(final UsbDevice device) {
                return attached.probeDriver(device);
            }

            @Override
            public boolean hasPermission(final UsbDevice device) {
                if (listing.get()) {
                    // What C++ may do on another thread while a port listing waits for Android's answer: a
                    // call that needs the devices' lock.
                    final Thread close = new Thread(() -> devices.close(7));
                    close.start();
                    try {
                        close.join(TimeUnit.SECONDS.toMillis(2));
                    } catch (final InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                    lockWasFree.set(!close.isAlive());
                }
                return attached.hasPermission(device);
            }

            @Override
            public void requestPermission(final UsbDevice device) {
                attached.requestPermission(device);
            }

            @Override
            public UsbDeviceConnection openDevice(final UsbDevice device) {
                return attached.openDevice(device);
            }

            @Override
            public boolean canAskUser() {
                return attached.canAskUser();
            }
        };
        devices.start(usbHost, callbacks);

        listing.set(true);
        assertEquals(1, devices.availablePorts().length);

        assertTrue(lockWasFree.get());
    }

    @Test
    public void usbEvents_tellCppWhenTheUsableDevicesMayHaveChanged() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks);
        assertEquals(0, callbacks.events.size());

        final UsbDevice adapter = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        usbHost.attach(adapter);
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, adapter);
        assertEquals(1, callbacks.events.size());

        // Neither a device without a serial driver nor a refused permission changes what can be opened.
        final UsbDevice mouse = new FakeUsbDevice(1004, "/dev/bus/usb/001/004");
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, mouse);
        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_DENIED, adapter);
        assertEquals(1, callbacks.events.size());

        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_GRANTED, adapter);
        // Accepting Android's own prompt for a device grants it without a broadcast.
        devices.onDeviceGranted(mouse);
        devices.onDeviceGranted(adapter);
        usbHost.drivers.remove(1);
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, adapter);
        assertEquals(List.of("devicesChanged", "devicesChanged", "devicesChanged", "devicesChanged"), callbacks.events);
    }

    @Test
    public void permission_isRequestedAtStartupButNotOnAttach() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final UsbDevice presentAtStartup = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(presentAtStartup);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());
        assertEquals(List.of(presentAtStartup), usbHost.permissionRequests);

        final UsbDevice attachedLater = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        usbHost.attach(attachedLater);
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, attachedLater);
        assertEquals(List.of(presentAtStartup), usbHost.permissionRequests);
    }

    @Test
    public void availablePorts_listsADeviceWithoutPermissionWithoutItsSerialNumber() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        device.serialNumber = "A1";
        usbHost.attach(device);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());
        // Android 9 would hand out the serial number without permission, so it must not even be asked for.
        assertEquals(fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME, "", "", "", "", 0, 0)),
                     fieldsOf(devices.availablePorts()));
        assertEquals(0, device.serialNumberReads);

        usbHost.grantsPermission = true;
        final List<List<Object>> expected =
                fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME, "usb:0000:0000:A1", "", "", "A1", 0, 0));
        assertEquals(expected, fieldsOf(devices.availablePorts()));
        final int permissionChecks = usbHost.permissionChecks;
        assertEquals(expected, fieldsOf(devices.availablePorts()));
        assertEquals(1, device.serialNumberReads);
        assertEquals(permissionChecks, usbHost.permissionChecks);
    }

    @Test
    public void port_isFoundByItsHandleWhileAnotherThreadHoldsTheDevicesLock() throws Exception {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final UsbDevice slowDevice = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        usbHost.attach(slowDevice);
        final CountDownLatch lockIsHeld = new CountDownLatch(1);
        final CountDownLatch lockMayBeReleased = new CountDownLatch(1);
        final AtomicBoolean lockWasReleased = new AtomicBoolean();
        // An open asks the host for a driver under the devices' lock.
        usbHost.onProbe = device -> {
            if (device != slowDevice) {
                return;
            }
            lockIsHeld.countDown();
            try {
                lockMayBeReleased.await(5, TimeUnit.SECONDS);
            } catch (final InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            lockWasReleased.set(true);
        };
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        final Thread slowOpen = new Thread(() -> devices.open(slowDevice.getDeviceName(), 8));
        slowOpen.start();
        assertTrue(lockIsHeld.await(5, TimeUnit.SECONDS));

        // Every write and command on an open port starts with this, and must not wait for the other thread.
        assertNotNull(devices.port(7));
        assertFalse(lockWasReleased.get());

        lockMayBeReleased.countDown();
        slowOpen.join(TimeUnit.SECONDS.toMillis(5));
    }

    @Test
    public void open_opensThePortOnADriverPortOfItsOwn() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final UsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean trackedPortOpen = usbHost.attach(device);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        // The host makes another driver for the device each time it is asked for one.
        usbHost.drivers.clear();
        final AtomicBoolean firstPortOpen = usbHost.attach(device);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertTrue(firstPortOpen.get());
        assertEquals(QGCUsbSerialResult.OK, devices.close(7));

        usbHost.drivers.clear();
        final AtomicBoolean secondPortOpen = usbHost.attach(device);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 8));
        assertTrue(secondPortOpen.get());
        // What an I/O thread left over from the first port would still call stays closed.
        assertFalse(firstPortOpen.get());
        assertFalse(trackedPortOpen.get());
    }

    @Test
    public void open_ofADeviceThatGetsNoDriverOfItsOwn_failsAndLeavesNoPort() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final UsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(device);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        usbHost.drivers.clear();
        assertEquals(QGCUsbSerialResult.FAILED, devices.open(DEVICE_NAME, 7));
        assertNull(devices.port(7));

        usbHost.attach(device);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
    }

    @Test
    public void usbEvent_whoseSyncWithTheDevicesFails_endsThereAndTheNextOneSyncsAgain() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks);
        final UsbDevice adapter = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(adapter);

        // Android's receiver hands each event on like this, with nothing above it to catch what the sync throws.
        usbHost.findDriversFailure = new RuntimeException(new InstantiationException("FtdiSerialDriver"));
        QGCUsbSerialAndroidHost.reportEvent(devices::onUsbEvent, QGCUsbSerialEvent.ATTACHED, adapter);
        assertEquals(List.of(), callbacks.events);

        usbHost.findDriversFailure = null;
        QGCUsbSerialAndroidHost.reportEvent(devices::onUsbEvent, QGCUsbSerialEvent.ATTACHED, adapter);
        assertEquals(List.of("devicesChanged"), callbacks.events);
    }

    @Test
    public void deviceGoneWithoutADetachEvent_losesItsPortOutsideTheSyncThatFindsItGone() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean driverPortOpen = usbHost.attach(board);
        final RecordingCallbacks recorded = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = new QGCUsbSerialDevices(recorded);
        final AtomicBoolean anotherSyncRanDuringTheCallback = new AtomicBoolean();
        devices.start(usbHost, new QGCUsbSerialCallbacks() {
            @Override
            public boolean newData(final long token, final byte[] data, final int length) {
                return recorded.newData(token, data, length);
            }

            @Override
            public void bytesWritten(final long token, final int count) {
                recorded.bytesWritten(token, count);
            }

            @Override
            public void resourceError(final long token, final int reason, final String detail) {
                recorded.resourceError(token, reason, detail);
                // C++ handles the error on this thread. Whatever syncs on another thread meanwhile, as the start
                // of the app does, must not have to wait for it.
                final Thread anotherSync = new Thread(() -> devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, null));
                anotherSync.start();
                try {
                    anotherSync.join(TimeUnit.SECONDS.toMillis(5));
                } catch (final InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
                anotherSyncRanDuringTheCallback.set(!anotherSync.isAlive());
            }

            @Override
            public void devicesChanged() {
                recorded.devicesChanged();
            }
        });
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));

        // The board is gone, and the event that tells of it has not arrived, when another device is attached.
        usbHost.drivers.clear();
        final UsbDevice adapter = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        usbHost.attach(adapter);
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, adapter);

        assertFalse(driverPortOpen.get());
        assertNull(devices.port(7));
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), recorded.events);
        assertTrue(anotherSyncRanDuringTheCallback.get());

        // The event that tells of the detach has nothing left to do.
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, board);
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), recorded.events);
    }

    @Test
    public void detach_whileThePortIsBeingOpened_closesItAndTellsCpp() throws Exception {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final UsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean driverPortOpen = new AtomicBoolean();
        final CountDownLatch opening = new CountDownLatch(1);
        final CountDownLatch deviceAnswers = new CountDownLatch(1);
        usbHost.drivers.add(driver(device, port(Map.of(
                "getDevice", args -> device,
                "isOpen", args -> driverPortOpen.get(),
                "open", args -> {
                    // The driver sets the device up with USB transfers, each of which waits for the device.
                    opening.countDown();
                    deviceAnswers.await(5, TimeUnit.SECONDS);
                    driverPortOpen.set(true);
                    return null;
                },
                "close", args -> {
                    driverPortOpen.set(false);
                    return null;
                }))));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks);
        final AtomicInteger openResult = new AtomicInteger(Integer.MIN_VALUE);
        final Thread opener = new Thread(() -> openResult.set(devices.open(DEVICE_NAME, 7)));
        opener.start();
        assertTrue(opening.await(5, TimeUnit.SECONDS));

        usbHost.drivers.clear();
        final Thread detach = new Thread(() -> devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, device));
        detach.start();
        // The detach cannot close the port before the driver is done with opening it.
        awaitTrue(() -> detach.getState() == Thread.State.BLOCKED);
        deviceAnswers.countDown();
        opener.join(TimeUnit.SECONDS.toMillis(5));
        detach.join(TimeUnit.SECONDS.toMillis(5));

        // C++ gets the port and hears at once that it is lost. No port is left behind that would refuse the next
        // opener of the device.
        assertEquals(QGCUsbSerialResult.OK, openResult.get());
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), callbacks.events);
        assertFalse(driverPortOpen.get());
        assertNull(devices.port(7));
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open(DEVICE_NAME, 8));
        assertNull(devices.port(8));
    }

    @Test
    public void usbEvent_thatNamesNoDevice_stillSyncsWithTheAttachedDevices() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final AtomicBoolean driverPortOpen = usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));

        // Nothing has changed, whatever the event.
        for (final QGCUsbSerialEvent event : QGCUsbSerialEvent.values()) {
            devices.onUsbEvent(event, null);
        }
        assertEquals(List.of(), callbacks.events);
        assertTrue(driverPortOpen.get());

        // A detach that names no device is found out by the sync.
        usbHost.drivers.clear();
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, null);
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), callbacks.events);
        assertFalse(driverPortOpen.get());
        assertEquals(0, devices.availablePorts().length);
    }

    @Test
    public void permission_canBeQueriedAndRequestedAgain() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final FakeUsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        device.productName = "FT232R";
        device.serialNumber = "A1";
        usbHost.attach(device);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        assertEquals(fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME, "", "FT232R", "", "", 0, 0)),
                     fieldsOf(devices.availablePorts()));
        assertFalse(devices.hasPermission(DEVICE_NAME));
        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals(List.of(device, device), usbHost.permissionRequests);
        assertFalse(devices.requestPermission("/dev/bus/usb/001/099"));

        usbHost.grantsPermission = true;
        assertEquals("usb:0000:0000:A1", devices.availablePorts()[0].stableName);
        assertTrue(devices.hasPermission(DEVICE_NAME));
        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals(2, usbHost.permissionRequests.size());
    }

    private static QGCUsbSerialDevices startDevices(final FakeUsbHost usbHost, final RecordingCallbacks callbacks) {
        final QGCUsbSerialDevices devices = new QGCUsbSerialDevices(callbacks);
        devices.start(usbHost, callbacks);
        return devices;
    }

    @Test
    public void availablePorts_listsTheDevicesWithAndWithoutPermission() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final AtomicLong clockMs = new AtomicLong();
        final FakeUsbDevice radio = new FakeUsbDevice(1002, DEVICE_NAME);
        radio.productName = "FT231X";
        radio.serialNumber = "R1";
        usbHost.attach(radio);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final FakeUsbDevice board = new FakeUsbDevice(1003, "/dev/bus/usb/001/003");
        board.productName = "CubeOrange+";
        board.serialNumber = "C7";
        usbHost.withoutPermission.add(board);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);

        final QGCUsbSerialPortInfo boardPort =
                new QGCUsbSerialPortInfo("/dev/bus/usb/001/003", "", "CubeOrange+", "", "", 0, 0);
        assertEquals(fieldsOf(new QGCUsbSerialPortInfo(DEVICE_NAME, "usb:0000:0000:R1", "FT231X", "", "R1", 0, 0),
                              boardPort),
                     fieldsOf(devices.availablePorts()));

        // Only the device that QGC may not open is asked for.
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 8));
        assertTrue(usbHost.permissionRequests.isEmpty());
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, "/dev/bus/usb/001/003"));
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void open_ofADeviceWithoutPermission_asksTheUserOnce() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);

        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);
        assertNull(devices.port(7));
        assertTrue(usbHost.connections.isEmpty());

        // C++ tries a board again every few seconds, which must not put a second dialog on top of the first.
        assertEquals("The user has not answered the request yet", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void open_afterAndroidClosedTheRequestWithoutAnAnswer_asksAgain() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));

        // QGC's window has the focus again and no answer has come: the dialog is gone.
        clockMs.addAndGet(QGCUsbSerialPermissionPrompt.REQUEST_GRACE_MS);
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board, board), usbHost.permissionRequests);
    }

    @Test
    public void open_afterAReplug_asksAgain() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_DENIED, board);

        // Android may give the device the ID and the name that it had.
        usbHost.drivers.clear();
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, board);
        final UsbDevice replugged = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(replugged);
        attachedAndPromptedByAndroid(devices, clockMs, replugged);

        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board, replugged), usbHost.permissionRequests);
    }

    @Test
    public void open_rightAfterTheAttach_leavesTheAskingToAndroid() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong(TimeUnit.HOURS.toMillis(1));
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, board);

        // C++ opens a board that it recognises within milliseconds, before Android's own prompt for it is up.
        assertEquals("The device was just attached, and Android asks the user first",
                     permissionDetailOfOpen(devices, DEVICE_NAME));
        assertTrue(usbHost.permissionRequests.isEmpty());

        clockMs.addAndGet(QGCUsbSerialPermissionPrompt.ATTACH_GRACE_MS);
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void open_whileAnotherWindowHasTheFocus_asksOnlyOnceQgcHasIt() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);

        // Android's prompt for the device is still up, or QGC is in the background.
        usbHost.windowFocused = false;
        assertEquals("The user is not asked while another window has the focus",
                     permissionDetailOfOpen(devices, DEVICE_NAME));
        assertTrue(usbHost.permissionRequests.isEmpty());

        usbHost.windowFocused = true;
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void open_afterTheUserGrantedTheRequest_succeeds() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        final QGCUsbSerialDevices devices = startDevices(usbHost, callbacks, clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean driverPortOpen = usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of("devicesChanged"), callbacks.events);

        usbHost.grantsPermission = true;
        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_GRANTED, board);

        // C++ hears of the grant and opens the port again.
        assertEquals(List.of("devicesChanged", "devicesChanged"), callbacks.events);
        assertEquals(QGCUsbSerialResult.OK, devices.open(DEVICE_NAME, 7));
        assertTrue(driverPortOpen.get());
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void requestPermission_asksAlsoAfterADenialAndCountsAsTheRequestOfAnOpen() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        attachedAndPromptedByAndroid(devices, clockMs, board);

        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals("The user has not answered the request yet", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);

        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_DENIED, board);
        assertEquals("The user denied it; replug the device or connect to it by hand to be asked again",
                     permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);

        assertTrue(devices.requestPermission(DEVICE_NAME));
        assertEquals(List.of(board, board), usbHost.permissionRequests);
        assertEquals("The user has not answered the request yet", permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board, board), usbHost.permissionRequests);
    }

    @Test
    public void permissionRequestAtStartup_countsAsTheRequestOfAnOpen() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        assertEquals(List.of(board), usbHost.permissionRequests);
        // The dialog of the request is up for as long as the user takes to answer it.
        usbHost.windowFocused = false;
        clockMs.addAndGet(QGCUsbSerialPermissionPrompt.ATTACH_GRACE_MS);

        assertEquals("The user has not answered the request yet", permissionDetailOfOpen(devices, DEVICE_NAME));
        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_DENIED, board);
        assertEquals("The user denied it; replug the device or connect to it by hand to be asked again",
                     permissionDetailOfOpen(devices, DEVICE_NAME));
        assertEquals(List.of(board), usbHost.permissionRequests);
    }

    @Test
    public void open_byAStableNameThatNoPermittedDeviceHas_asksForTheOnlyDeviceOfItsKindWithoutPermission() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final AtomicLong clockMs = new AtomicLong();
        final FakeUsbDevice permitted = twoPortAdapter(1002, DEVICE_NAME);
        permitted.serialNumber = "B2";
        usbHost.attach(permitted, 2);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final FakeUsbDevice adapter = twoPortAdapter(1003, "/dev/bus/usb/001/003");
        usbHost.withoutPermission.add(adapter);
        final List<AtomicBoolean> adapterPortsOpen = usbHost.attach(adapter, 2);
        attachedAndPromptedByAndroid(devices, clockMs, adapter);

        // Not the adapter's port, its kind, or the name of the adapter that QGC may open.
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6010:A1#p2", 7));
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6011:A1#p1", 7));
        assertEquals(QGCUsbSerialResult.OK, devices.open("usb:0403:6010:B2#p1", 8));
        assertTrue(usbHost.permissionRequests.isEmpty());

        // The name that a link has of the adapter from when QGC could open it. Android withholds the serial
        // number that would tell that it is this adapter's.
        assertEquals("The user has been asked to allow it", permissionDetailOfOpen(devices, "usb:0403:6010:A1#p1"));
        assertEquals(List.of(adapter), usbHost.permissionRequests);
        assertTrue(devices.requestPermission("usb:0403:6010:A1#p1"));
        assertEquals(List.of(adapter, adapter), usbHost.permissionRequests);

        usbHost.withoutPermission.remove(adapter);
        devices.onUsbEvent(QGCUsbSerialEvent.PERMISSION_GRANTED, adapter);
        assertEquals(QGCUsbSerialResult.OK, devices.open("usb:0403:6010:A1#p1", 7));
        assertTrue(adapterPortsOpen.get(1).get());
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6010:C3#p1", 9));
    }

    @Test
    public void open_byAStableNameThatSeveralDevicesWithoutPermissionCouldHave_findsNoDevice() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final AtomicLong clockMs = new AtomicLong();
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks(), clockMs);
        final UsbDevice adapter = twoPortAdapter(1002, DEVICE_NAME);
        usbHost.attach(adapter, 2);
        attachedAndPromptedByAndroid(devices, clockMs, adapter);
        final UsbDevice another = twoPortAdapter(1003, "/dev/bus/usb/001/003");
        usbHost.attach(another, 2);
        attachedAndPromptedByAndroid(devices, clockMs, another);

        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, devices.open("usb:0403:6010:A1#p1", 7));
        assertFalse(devices.requestPermission("usb:0403:6010:A1#p1"));
        assertTrue(usbHost.permissionRequests.isEmpty());

        // Each still has the name that Android gave it.
        assertEquals("The user has been asked to allow it",
                     permissionDetailOfOpen(devices, "/dev/bus/usb/001/003#p1"));
        assertEquals(List.of(another), usbHost.permissionRequests);
    }

    @Test
    public void open_byAStableNameThatTwoDevicesShare_opensNeither() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        // Adapters of a kind that report the same serial number, or none, have the same stable name.
        usbHost.attach(twoPortAdapter(1002, DEVICE_NAME), 2);
        final FakeUsbDevice twin = twoPortAdapter(1003, "/dev/bus/usb/001/003");
        final List<AtomicBoolean> twinPortsOpen = usbHost.attach(twin, 2);
        final QGCUsbSerialDevices devices = startDevices(usbHost, new RecordingCallbacks());

        // Either could be meant, and the other one may be on another vehicle.
        QGCUsbSerialLastError.clear();
        assertEquals(QGCUsbSerialResult.FAILED, devices.open("usb:0403:6010:A1#p1", 7));
        assertEquals("2 attached devices have this name, so it opens none of them", QGCUsbSerialLastError.get());
        assertFalse(devices.hasPermission("usb:0403:6010:A1#p1"));
        assertFalse(devices.requestPermission("usb:0403:6010:A1#p1"));

        // Each still opens by the name that Android gave it.
        assertEquals(QGCUsbSerialResult.OK, devices.open("/dev/bus/usb/001/003#p1", 7));
        assertTrue(twinPortsOpen.get(1).get());
        assertEquals(QGCUsbSerialResult.OK, devices.close(7));

        // And by the stable name again once only one device has it.
        usbHost.drivers.remove(1);
        devices.onUsbEvent(QGCUsbSerialEvent.DETACHED, twin);
        assertEquals(QGCUsbSerialResult.OK, devices.open("usb:0403:6010:A1#p1", 8));
    }

    /** Starts devices that tell the time by the given clock, in milliseconds. */
    private static QGCUsbSerialDevices startDevices(final FakeUsbHost usbHost, final RecordingCallbacks callbacks,
                                                    final AtomicLong clockMs) {
        final QGCUsbSerialDevices devices = new QGCUsbSerialDevices(callbacks, clockMs::get);
        devices.start(usbHost, callbacks);
        return devices;
    }

    /**
     * Reports a device that was attached while QGC runs, and lets the time pass that Android's own prompt for it
     * takes to appear. From then on QGC may ask for the device itself, as it must once the user has dismissed
     * that prompt.
     */
    private static void attachedAndPromptedByAndroid(final QGCUsbSerialDevices devices, final AtomicLong clockMs,
                                                     final UsbDevice device) {
        devices.onUsbEvent(QGCUsbSerialEvent.ATTACHED, device);
        clockMs.addAndGet(QGCUsbSerialPermissionPrompt.ATTACH_GRACE_MS);
    }

    /** Opens the port, which must be refused for lack of permission, and returns the failure detail that leaves. */
    private static String permissionDetailOfOpen(final QGCUsbSerialDevices devices, final String name) {
        // As QGCUsbSerialManager.open() forgets the detail of the call before.
        QGCUsbSerialLastError.clear();
        assertEquals(QGCUsbSerialResult.PERMISSION_DENIED, devices.open(name, 7));
        return QGCUsbSerialLastError.get();
    }
}
