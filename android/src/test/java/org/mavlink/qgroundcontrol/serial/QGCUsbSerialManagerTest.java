package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.DEVICE_NAME;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.awaitTrue;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.driver;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.port;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.portOnAnIdleLine;

import android.hardware.usb.FakeUsbDevice;
import android.hardware.usb.UsbDevice;
import com.hoho.android.usbserial.driver.SerialTimeoutException;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import java.io.IOException;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.LongToIntFunction;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.FakeUsbHost;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.PortMethod;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialTestSupport.RecordingCallbacks;

public class QGCUsbSerialManagerTest {
    // Where openPort() attaches the devices of the ports it opens.
    private final FakeUsbHost _usbHost = new FakeUsbHost();
    // What the ports that openPort() opens report to.
    private final RecordingCallbacks _callbacks = new RecordingCallbacks();

    @Before
    public void setUp() {
        QGCUsbSerialManager.cleanup();
    }

    @After
    public void tearDown() {
        QGCUsbSerialManager.cleanup();
    }

    @Test
    public void cleanup_closesThePortsAndForgetsTheirHandles() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final AtomicBoolean driverPortOpen = usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));

        QGCUsbSerialManager.cleanup();
        assertFalse(driverPortOpen.get());
        assertEquals(QGCUsbSerialResult.NOT_OPEN, QGCUsbSerialManager.setBreak(7, true));

        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));
    }

    @Test
    public void startReadingAndStopReading_reportOkAndIsReadingFollowsThem() {
        final Semaphore readsToEnd = new Semaphore(0);
        final long handle = openPort(7, portOnAnIdleLine(readsToEnd));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.startReading(handle));
        assertTrue(QGCUsbSerialManager.isReading(handle));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.stopReading(handle));
        assertFalse(QGCUsbSerialManager.isReading(handle));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(handle));
        readsToEnd.release(100);
    }

    @Test
    public void detach_endsAReadThreadThatWaitsForCppToHaveRoom() throws Exception {
        final AtomicReference<Thread> readThread = new AtomicReference<>();
        // A line that has input for every read. It is read no further once C++ has no room.
        final long handle = openPort(7, port(Map.of(
                "setReadQueue", args -> null,
                "read", args -> {
                    readThread.set(Thread.currentThread());
                    return 1;
                })));
        _callbacks.hasRoom = false;
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.startReading(handle));
        awaitTrue(() -> readThread.get() != null && readThread.get().getState() == Thread.State.WAITING);
        assertTrue(QGCUsbSerialManager.isReading(handle));

        final UsbDevice device = _usbHost.drivers.get(0).getDevice();
        _usbHost.drivers.clear();
        QGCUsbSerialManager.onUsbEvent(QGCUsbSerialEvent.DETACHED, device);

        readThread.get().join(TimeUnit.SECONDS.toMillis(5));
        assertFalse(readThread.get().isAlive());
        assertEquals(List.of("data 7 1", "resourceError 7 1 []", "devicesChanged"), _callbacks.events);
    }

    @Test
    public void usbEvents_arePassedOnToTheDevices() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        final RecordingCallbacks callbacks = new RecordingCallbacks();
        QGCUsbSerialManager.initializeForTesting(usbHost, callbacks);

        final UsbDevice adapter = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(adapter);
        QGCUsbSerialManager.onUsbEvent(QGCUsbSerialEvent.ATTACHED, adapter);
        assertEquals(List.of("devicesChanged"), callbacks.events);

        QGCUsbSerialManager.onUsbDeviceGranted(adapter);
        assertEquals(List.of("devicesChanged", "devicesChanged"), callbacks.events);
    }

    @Test
    public void deviceQueries_arePassedOnToTheDevices() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());

        assertEquals(1, QGCUsbSerialManager.availablePorts().length);
        assertTrue(QGCUsbSerialManager.hasPermission(DEVICE_NAME));
        assertTrue(QGCUsbSerialManager.requestPermission(DEVICE_NAME));

        assertEquals(0, QGCUsbSerialManager.getPortHandle(DEVICE_NAME));
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals(7, QGCUsbSerialManager.getPortHandle(DEVICE_NAME));
        assertEquals(0, QGCUsbSerialManager.getPortHandle("/dev/bus/usb/001/099"));
    }

    @Test
    public void usbSerialEnums_crossJniAsTheNumbersCppLists() {
        final UsbSerialPort.FlowControl[] flowControl = {UsbSerialPort.FlowControl.NONE};
        final UsbSerialPort.ControlLine[] activeLine = {UsbSerialPort.ControlLine.RTS};
        final UsbSerialPort port = port(Map.of(
                "getFlowControl", args -> flowControl[0],
                "getControlLines", args -> EnumSet.of(activeLine[0])));
        final long portId = openPort(7, port);

        // The order of AndroidSerial::FlowControl.
        final List<UsbSerialPort.FlowControl> flowControls = List.of(
                UsbSerialPort.FlowControl.NONE,
                UsbSerialPort.FlowControl.RTS_CTS,
                UsbSerialPort.FlowControl.DTR_DSR,
                UsbSerialPort.FlowControl.XON_XOFF,
                UsbSerialPort.FlowControl.XON_XOFF_INLINE);
        for (int code = 0; code < flowControls.size(); code++) {
            flowControl[0] = flowControls.get(code);
            assertEquals(code, QGCUsbSerialManager.getFlowControl(portId));
        }
        // The order of kSignals in AndroidSerial.cc.
        final List<UsbSerialPort.ControlLine> controlLines = List.of(
                UsbSerialPort.ControlLine.RTS,
                UsbSerialPort.ControlLine.CTS,
                UsbSerialPort.ControlLine.DTR,
                UsbSerialPort.ControlLine.DSR,
                UsbSerialPort.ControlLine.CD,
                UsbSerialPort.ControlLine.RI);
        for (int code = 0; code < controlLines.size(); code++) {
            activeLine[0] = controlLines.get(code);
            assertArrayEquals(new int[] {code}, QGCUsbSerialManager.getControlLines(portId));
        }
    }

    @Test
    public void capabilities_reportWhatTheDriverSupports() {
        final UsbSerialPort port = port(Map.of(
                "getSupportedFlowControl",
                args -> EnumSet.of(UsbSerialPort.FlowControl.NONE, UsbSerialPort.FlowControl.XON_XOFF),
                "getSupportedControlLines",
                args -> EnumSet.of(UsbSerialPort.ControlLine.RTS, UsbSerialPort.ControlLine.DTR),
                "getXON", args -> {
                    throw new UnsupportedOperationException();
                }));
        final long portId = openPort(7, port);

        assertArrayEquals(new int[] {0, 3}, QGCUsbSerialManager.getSupportedFlowControl(portId));
        assertArrayEquals(new int[] {0, 2}, QGCUsbSerialManager.getSupportedControlLines(portId));
        assertTrue(QGCUsbSerialManager.getXon(portId));
        // Only FTDI ports have a latency timer.
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setLatencyTimer(portId, 8));
    }

    @Test
    public void setParameters_passesThemOnAndReportsWhatTheDriverRejects() {
        final List<String> applied = new ArrayList<>();
        final UsbSerialPort port = port(Map.of(
                "setParameters", args -> {
                    if ((Integer) args[0] == 1) {
                        throw new UnsupportedOperationException("Unsupported baud rate");
                    }
                    if ((Integer) args[0] == 2) {
                        throw new IOException("Disconnected");
                    }
                    applied.add(args[0] + " baud, " + args[1] + " data, " + args[2] + " stop, parity " + args[3]);
                    return null;
                }));
        final long portId = openPort(7, port);

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.setParameters(portId, 57600, 8, 1, 2));
        assertEquals(List.of("57600 baud, 8 data, 1 stop, parity 2"), applied);
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setParameters(portId, 1, 8, 1, 0));
        assertEquals(QGCUsbSerialResult.IO_ERROR, QGCUsbSerialManager.setParameters(portId, 2, 8, 1, 0));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(portId));
        assertEquals(QGCUsbSerialResult.NOT_OPEN, QGCUsbSerialManager.setParameters(portId, 57600, 8, 1, 0));
        assertEquals(1, applied.size());
    }

    @Test
    public void setFlowControl_changesOnlyToWhatThePortSupports() {
        final List<UsbSerialPort.FlowControl> sent = new ArrayList<>();
        final UsbSerialPort.FlowControl[] current = {UsbSerialPort.FlowControl.NONE};
        final UsbSerialPort port = port(Map.of(
                "getFlowControl", args -> current[0],
                "getSupportedFlowControl",
                args -> EnumSet.of(UsbSerialPort.FlowControl.NONE, UsbSerialPort.FlowControl.RTS_CTS),
                "setFlowControl", args -> {
                    current[0] = (UsbSerialPort.FlowControl) args[0];
                    sent.add(current[0]);
                    return null;
                }));
        final long portId = openPort(7, port);

        // The values are AndroidSerial::FlowControl: 1 is RTS/CTS, 3 is XON/XOFF, and there are five.
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.setFlowControl(portId, 1));
        assertEquals(1, QGCUsbSerialManager.getFlowControl(portId));
        // Setting what is already set sends nothing to the device.
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.setFlowControl(portId, 1));
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setFlowControl(portId, 3));
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setFlowControl(portId, 5));
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setFlowControl(portId, -1));
        assertEquals(List.of(UsbSerialPort.FlowControl.RTS_CTS), sent);
        assertEquals(1, QGCUsbSerialManager.getFlowControl(portId));
    }

    @Test
    public void controlLines_areUsedOnlyWhereThePortHasThem() {
        final List<String> sent = new ArrayList<>();
        // No handler for RTS: touching that line would fail the test.
        final UsbSerialPort port = port(Map.of(
                "getSupportedControlLines",
                args -> EnumSet.of(UsbSerialPort.ControlLine.DTR, UsbSerialPort.ControlLine.CTS),
                "getControlLines", args -> EnumSet.of(UsbSerialPort.ControlLine.DTR),
                "setDTR", args -> {
                    sent.add("DTR " + args[0]);
                    return null;
                }));
        final long portId = openPort(7, port);

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.setDataTerminalReady(portId, true));
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setRequestToSend(portId, true));
        assertEquals(List.of("DTR true"), sent);

        // AndroidSerial::ControlLine: 2 is DTR.
        assertArrayEquals(new int[] {2}, QGCUsbSerialManager.getControlLines(portId));
    }

    @Test
    public void purgeBuffers_passesWriteThenReadToPort() {
        final boolean[] purged = new boolean[2];
        final UsbSerialPort port = port(Map.of(
                "purgeHwBuffers", args -> {
                    purged[0] = (Boolean) args[0];
                    purged[1] = (Boolean) args[1];
                    return null;
                }));
        final long portId = openPort(7, port);

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.purgeBuffers(portId, true, false));
        assertArrayEquals(new boolean[] {false, true}, purged);
    }

    @Test
    public void purgeBuffers_driverWithoutPurge_succeeds() {
        final UsbSerialPort port = port(Map.of(
                "purgeHwBuffers", args -> {
                    throw new UnsupportedOperationException();
                }));
        final long portId = openPort(7, port);

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.purgeBuffers(portId, true, true));
    }

    @Test
    public void purgeBuffers_driverWithoutPurge_stillStopsTheOutputBeingWritten() throws Exception {
        final List<Integer> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final long portId = openPort(7, port(Map.of(
                "purgeHwBuffers", args -> {
                    throw new UnsupportedOperationException();
                },
                "write", args -> {
                    writes.add((Integer) args[1]);
                    deviceTakesData.acquire();
                    return null;
                })));

        assertEquals(2500, QGCUsbSerialManager.writeAsync(portId, new byte[2500], 2500));
        awaitTrue(() -> writes.size() == 1);
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.purgeBuffers(portId, false, true));

        deviceTakesData.release(100);
        awaitTrue(() -> !_callbacks.events.isEmpty());
        assertEquals(List.of("written 7 2500"), _callbacks.events);
        assertEquals(List.of(1024), writes);
    }

    @Test
    public void writeWithProgress_separatesLostDeviceFromTimeout() {
        final byte[] data = new byte[8];
        final UsbSerialPort timingOut = port(Map.of("write", args -> {
            throw new SerialTimeoutException("Timed out", 5);
        }));
        assertEquals(5, QGCUsbSerialManager.writeWithProgress(openPort(7, timingOut), data, data.length, 100));

        final UsbSerialPort unplugged = port(Map.of("write", args -> {
            throw new IOException("Error writing 8 bytes, rc=-1");
        }));
        final long unpluggedId = openPort(8, unplugged);
        assertEquals(QGCUsbSerialResult.IO_ERROR,
                     QGCUsbSerialManager.writeWithProgress(unpluggedId, data, data.length, 100));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(unpluggedId));
        assertEquals(QGCUsbSerialResult.NOT_OPEN,
                     QGCUsbSerialManager.writeWithProgress(unpluggedId, data, data.length, 100));
    }

    @Test
    public void writeAsync_afterThePortIsClosed_isNotOpen() {
        final long handle = openPort(7, port(Map.of()));

        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(handle));
        assertEquals(QGCUsbSerialResult.NOT_OPEN, QGCUsbSerialManager.writeAsync(handle, new byte[1], 1));
    }

    @Test
    public void command_onAPortClosedUnderIt_isNotOpen() {
        final long handle = openPort(7, port(Map.of("setBreak", failingAsThePortCloses(7))));

        assertEquals(QGCUsbSerialResult.NOT_OPEN, QGCUsbSerialManager.setBreak(handle, true));
    }

    @Test
    public void query_onAPortClosedUnderIt_returnsItsFallback() {
        final long handle = openPort(7, port(Map.of("getControlLines", failingAsThePortCloses(7))));
        assertArrayEquals(new int[0], QGCUsbSerialManager.getControlLines(handle));
    }

    @Test
    public void driverException_onAPortThatIsStillOpen_propagates() {
        // What a closed port's driver throws too, so only the state of the port tells the two apart.
        final PortMethod defect = args -> {
            throw new NullPointerException("Driver defect");
        };
        final long handle = openPort(7, port(Map.of(
                "setBreak", defect,
                "getControlLines", defect,
                "getSupportedControlLines", defect)));

        assertThrows(NullPointerException.class, () -> QGCUsbSerialManager.setBreak(handle, true));
        assertThrows(NullPointerException.class, () -> QGCUsbSerialManager.getControlLines(handle));
        assertThrows(NullPointerException.class, () -> QGCUsbSerialManager.getSupportedControlLines(handle));
    }

    /** A call that fails because the driver's method throws, with what the call then returns. */
    private record FailedCall(String driverMethod, Exception failure, LongToIntFunction call, int result) {}

    @Test
    public void getLastErrorDetail_isWhatTheDriverSaidOfTheFailedCall() {
        final byte[] data = new byte[8];
        final List<FailedCall> failedCalls = List.of(
                new FailedCall("setParameters", new UnsupportedOperationException("Unsupported baud rate"),
                        handle -> QGCUsbSerialManager.setParameters(handle, 1, 8, 1, 0),
                        QGCUsbSerialResult.UNSUPPORTED),
                new FailedCall("setBreak", new IOException("Disconnected"),
                        handle -> QGCUsbSerialManager.setBreak(handle, true), QGCUsbSerialResult.IO_ERROR),
                // An exception without a message leaves the result to speak for itself.
                new FailedCall("purgeHwBuffers", new IOException(),
                        handle -> QGCUsbSerialManager.purgeBuffers(handle, true, true), QGCUsbSerialResult.IO_ERROR),
                new FailedCall("setReadQueue", new IllegalStateException("Cannot change bufferSize when port is open"),
                        QGCUsbSerialManager::startReading, QGCUsbSerialResult.FAILED),
                new FailedCall("write", new IOException("Error writing 8 bytes, rc=-1"),
                        handle -> QGCUsbSerialManager.writeWithProgress(handle, data, data.length, 100),
                        QGCUsbSerialResult.IO_ERROR),
                // A write that times out returns what it wrote, and still says how it ended.
                new FailedCall("write", new SerialTimeoutException("Error writing 3 bytes at offset 5", 5),
                        handle -> QGCUsbSerialManager.writeWithProgress(handle, data, data.length, 100), 5),
                // A query returns its fallback, here no lines.
                new FailedCall("getControlLines", new IOException("Disconnected"),
                        handle -> QGCUsbSerialManager.getControlLines(handle).length, 0));

        long opened = 0;
        for (final FailedCall failed : failedCalls) {
            final long handle = ++opened;
            openPort(handle, port(Map.of(failed.driverMethod(), args -> {
                throw failed.failure();
            })));
            assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

            assertEquals(failed.driverMethod(), failed.result(), failed.call().applyAsInt(handle));
            final String said = failed.failure().getMessage();
            assertEquals(failed.driverMethod(), (said != null) ? said : "", QGCUsbSerialManager.getLastErrorDetail());
        }
    }

    @Test
    public void getLastErrorDetail_isForgottenByTheNextCall() {
        final long handle = openPort(7, port(Map.of(
                "setBreak", args -> {
                    throw new IOException("Disconnected");
                },
                "getControlLines", args -> EnumSet.noneOf(UsbSerialPort.ControlLine.class),
                "setParameters", args -> null,
                "write", args -> null)));
        final Runnable failACommand = () -> {
            assertEquals(QGCUsbSerialResult.IO_ERROR, QGCUsbSerialManager.setBreak(handle, true));
            assertEquals("Disconnected", QGCUsbSerialManager.getLastErrorDetail());
        };

        failACommand.run();
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.setParameters(handle, 57600, 8, 1, 0));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

        failACommand.run();
        assertArrayEquals(new int[0], QGCUsbSerialManager.getControlLines(handle));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

        failACommand.run();
        assertEquals(1, QGCUsbSerialManager.writeWithProgress(handle, new byte[1], 1, 100));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

        // A failure that has nothing to add must not pass off the earlier one's detail as its own.
        failACommand.run();
        assertEquals(QGCUsbSerialResult.UNSUPPORTED, QGCUsbSerialManager.setFlowControl(handle, 5));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

        failACommand.run();
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(handle));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void getLastErrorDetail_isKeptForTheThreadOfTheFailedCall() throws Exception {
        final long handle = openPort(7, port(Map.of("setBreak", args -> {
            throw new IOException("Disconnected");
        })));
        assertEquals(QGCUsbSerialResult.IO_ERROR, QGCUsbSerialManager.setBreak(handle, true));

        // As when another C++ port's thread calls between the failure and the question about it.
        final String[] onAnotherThread = new String[1];
        final Thread otherThread = new Thread(() -> {
            onAnotherThread[0] = QGCUsbSerialManager.getLastErrorDetail();
            QGCUsbSerialManager.setBreak(99, true);
        });
        otherThread.start();
        otherThread.join();

        assertEquals("", onAnotherThread[0]);
        assertEquals("Disconnected", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void getLastErrorDetail_afterAFailedOpen_tellsWhy() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());

        usbHost.driverOpenFailure = new IOException("Could not claim the interface");
        assertEquals(QGCUsbSerialResult.FAILED, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals("Could not claim the interface", QGCUsbSerialManager.getLastErrorDetail());

        // Where no exception tells the reason, a short text does.
        usbHost.driverOpenFailure = null;
        usbHost.opensConnections = false;
        assertEquals(QGCUsbSerialResult.FAILED, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals("Android opened no USB connection", QGCUsbSerialManager.getLastErrorDetail());
        assertEquals(QGCUsbSerialResult.FAILED, QGCUsbSerialManager.open(DEVICE_NAME, 0));
        assertEquals("Unusable port handle 0", QGCUsbSerialManager.getLastErrorDetail());

        // The result says all there is to say.
        assertEquals(QGCUsbSerialResult.DEVICE_NOT_FOUND, QGCUsbSerialManager.open("/dev/bus/usb/001/099", 7));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void getLastErrorDetail_afterAFailedClose_isWhatTheDriverSaid() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.attach(new FakeUsbDevice(1002, DEVICE_NAME));
        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));

        usbHost.driverCloseFailure = new IOException("Already closed");
        assertEquals(QGCUsbSerialResult.IO_ERROR, QGCUsbSerialManager.close(7));
        assertEquals("Already closed", QGCUsbSerialManager.getLastErrorDetail());

        // The port is forgotten all the same, so closing it again succeeds.
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.close(7));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void getLastErrorDetail_afterAWriteOnAPortClosedUnderIt_tellsOfTheClose() {
        final long handle = openPort(7, port(Map.of("write", failingAsThePortCloses(7))));

        // Not what the driver's exception says, which is only how it stumbled over the cleared connection.
        assertEquals(QGCUsbSerialResult.IO_ERROR,
                     QGCUsbSerialManager.writeWithProgress(handle, new byte[8], 8, 100));
        assertEquals("The port was closed during the write", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void getLastErrorDetail_afterARefusedWriteAsync_tellsThatTheWriteThreadIsBehind() throws Exception {
        final List<Integer> writes = new CopyOnWriteArrayList<>();
        final Semaphore deviceTakesData = new Semaphore(0);
        final long handle = openPort(7, port(Map.of("write", args -> {
            writes.add((Integer) args[1]);
            deviceTakesData.acquire();
            return null;
        })));

        // One chunk is with the device, and four more wait behind it.
        assertEquals(1, QGCUsbSerialManager.writeAsync(handle, new byte[1], 1));
        awaitTrue(() -> writes.size() == 1);
        for (int waiting = 0; waiting < 4; ++waiting) {
            assertEquals(1, QGCUsbSerialManager.writeAsync(handle, new byte[1], 1));
        }
        assertEquals(QGCUsbSerialResult.FAILED, QGCUsbSerialManager.writeAsync(handle, new byte[1], 1));
        assertEquals("The write thread still has 4 chunks to write", QGCUsbSerialManager.getLastErrorDetail());

        deviceTakesData.release(100);
        awaitTrue(() -> _callbacks.events.size() == 5);
        assertEquals(1, QGCUsbSerialManager.writeAsync(handle, new byte[1], 1));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }

    @Test
    public void startReading_onAPortClosedUnderIt_isNotOpen() {
        final UsbDevice device = new FakeUsbDevice(1002, DEVICE_NAME);
        final AtomicBoolean open = new AtomicBoolean();
        final AtomicBoolean detachesNow = new AtomicBoolean();
        _usbHost.drivers.add(driver(device, port(Map.of(
                "getDevice", args -> device,
                "open", args -> {
                    open.set(true);
                    return null;
                },
                "close", args -> {
                    open.set(false);
                    return null;
                },
                "isOpen", args -> {
                    final boolean wasOpen = open.get();
                    // The detach is handled on another thread, right after the port was found to be open.
                    if (detachesNow.compareAndSet(true, false)) {
                        _usbHost.drivers.clear();
                        QGCUsbSerialManager.onUsbEvent(QGCUsbSerialEvent.DETACHED, device);
                    }
                    return wasOpen;
                }))));
        QGCUsbSerialManager.initializeForTesting(_usbHost, _callbacks);
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));

        detachesNow.set(true);
        // C++ takes this result for the loss of the device, and any other failure for one to start reading.
        assertEquals(QGCUsbSerialResult.NOT_OPEN, QGCUsbSerialManager.startReading(7));
        assertEquals(List.of("resourceError 7 1 []", "devicesChanged"), _callbacks.events);
    }

    @Test
    public void getLastErrorDetail_isForgottenByACallThatFindsNoPortOrDevice() {
        final long handle = openPort(7, port(Map.of("setBreak", args -> {
            throw new IOException("Disconnected");
        })));
        final Runnable failACommand = () -> {
            assertEquals(QGCUsbSerialResult.IO_ERROR, QGCUsbSerialManager.setBreak(handle, true));
            assertEquals("Disconnected", QGCUsbSerialManager.getLastErrorDetail());
        };

        failACommand.run();
        assertEquals(-1, QGCUsbSerialManager.getFileDescriptor(99));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());

        failACommand.run();
        assertFalse(QGCUsbSerialManager.requestPermission("/dev/bus/usb/001/099"));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }

    /**
     * Opens a port as C++ does, under {@code handle} and on a device of its own, whose driver port does what
     * {@code driverPort} does once it is open. Returns the handle.
     */
    private long openPort(final long handle, final UsbSerialPort driverPort) {
        final UsbDevice device = new FakeUsbDevice((int) handle, "/dev/bus/usb/001/00" + handle);
        _usbHost.attach(device, driverPort);
        // Starting the facade again, for a further port of a test, makes it find the device just attached.
        QGCUsbSerialManager.initializeForTesting(_usbHost, _callbacks);
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(device.getDeviceName(), handle));
        return handle;
    }

    /**
     * What a driver's method does when a detach, handled on another thread, closes the port under it: the driver
     * fails on the USB connection that the close cleared.
     */
    private static PortMethod failingAsThePortCloses(final long handle) {
        return args -> {
            QGCUsbSerialManager.close(handle);
            throw new NullPointerException("Connection cleared by close");
        };
    }

    @Test
    public void getLastErrorDetail_afterAnOpenWithoutPermission_tellsWhatBecameOfTheRequestForIt() {
        final FakeUsbHost usbHost = new FakeUsbHost();
        usbHost.grantsPermission = false;
        final UsbDevice board = new FakeUsbDevice(1002, DEVICE_NAME);
        usbHost.attach(board);
        // Starting asks for the devices that are attached.
        QGCUsbSerialManager.initializeForTesting(usbHost, new RecordingCallbacks());
        assertEquals(1, QGCUsbSerialManager.availablePorts().length);
        assertEquals("", QGCUsbSerialManager.availablePorts()[0].stableName);

        assertEquals(QGCUsbSerialResult.PERMISSION_DENIED, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals("The user has not answered the request yet", QGCUsbSerialManager.getLastErrorDetail());

        QGCUsbSerialManager.onUsbEvent(QGCUsbSerialEvent.PERMISSION_DENIED, board);
        assertEquals(QGCUsbSerialResult.PERMISSION_DENIED, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals("The user denied it; replug the device or connect to it by hand to be asked again",
                     QGCUsbSerialManager.getLastErrorDetail());

        assertTrue(QGCUsbSerialManager.requestPermission(DEVICE_NAME));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
        assertEquals(List.of(board, board), usbHost.permissionRequests);

        usbHost.grantsPermission = true;
        QGCUsbSerialManager.onUsbEvent(QGCUsbSerialEvent.PERMISSION_GRANTED, board);
        assertEquals(QGCUsbSerialResult.OK, QGCUsbSerialManager.open(DEVICE_NAME, 7));
        assertEquals("", QGCUsbSerialManager.getLastErrorDetail());
    }
}
