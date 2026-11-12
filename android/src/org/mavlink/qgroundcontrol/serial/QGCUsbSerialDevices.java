package org.mavlink.qgroundcontrol.serial;

import android.hardware.usb.UsbDevice;
import android.os.SystemClock;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import org.mavlink.qgroundcontrol.QGCLogger;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.LongSupplier;

/**
 * The attached USB serial devices and the ports opened on them.
 *
 * <p>{@link QGCUsbSerialPortName} describes how a port is named. C++ gives each port it opens a
 * handle of its choosing, which is never 0, and names the port by it until it closes the port.
 *
 * <p>{@code _lock} guards only the tracked devices and the changes of the table of open ports. USB
 * transfers, including those that open and close a port, calls into Android's USB service and callbacks
 * into C++ run without it: a transfer can block for seconds, and the C++ handlers can call back into this
 * class. Nor does finding a port by its handle take it, which every call on an open port starts with.
 */
final class QGCUsbSerialDevices {
    // Spelled out, because a release build renames the class and the tag is what a log is filtered by.
    private static final String TAG = "QGCUsbSerialDevices";
    private static final long NO_HANDLE = 0;

    record PortAddress(int usbDeviceId, int portIndex) {}

    /**
     * An attached device that the library has a driver for. Of that driver only the number of its ports is
     * kept: each port that is opened gets a driver of its own ({@link #open}).
     */
    private final class TrackedDevice {
        final UsbDevice device;
        final int portCount;
        final QGCUsbSerialPermissionPrompt prompt = new QGCUsbSerialPermissionPrompt(_clockMs.getAsLong());
        // Both take a call into the USB service, so the device poll asks only until permission is granted,
        // which then lasts until the device detaches.
        boolean permitted;
        String serialNumber;

        TrackedDevice(final UsbSerialDriver driver) {
            device = driver.getDevice();
            portCount = driver.getPorts().size();
        }

        boolean hasPort(final QGCUsbSerialPortName portName) {
            return portName.portIndex() < portCount;
        }
    }

    private final Object _lock = new Object();
    // Held for a whole sync of the tracked devices. The start of the app and the USB events sync from different
    // threads, and a sync that read the attached devices earlier must not replace what a later one found.
    private final Object _refreshLock = new Object();
    private final QGCUsbSerialCallbacks _defaultCallbacks;
    // Milliseconds that only ever increase, to tell how long a device has been attached.
    private final LongSupplier _clockMs;
    private QGCUsbSerialHost _usbHost;
    private QGCUsbSerialCallbacks _callbacks;
    private final Map<Integer, TrackedDevice> _devices = new LinkedHashMap<>(); // by USB device ID
    // By handle. Changed and searched only under _lock, but read by handle without it, which every call on an
    // open port starts with.
    private final Map<Long, QGCUsbSerialPort> _ports = new ConcurrentHashMap<>();
    private int _nextPortId = 1;

    /**
     * Before {@link #start} and after {@link #stop} no device is attached, and ports report to
     * {@code defaultCallbacks}.
     */
    QGCUsbSerialDevices(final QGCUsbSerialCallbacks defaultCallbacks) {
        this(defaultCallbacks, SystemClock::elapsedRealtime);
    }

    /** As the other constructor, with a clock in place of Android's, for unit tests. */
    QGCUsbSerialDevices(final QGCUsbSerialCallbacks defaultCallbacks, final LongSupplier clockMs) {
        _defaultCallbacks = defaultCallbacks;
        _callbacks = defaultCallbacks;
        _clockMs = clockMs;
    }

    // ------------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------------

    void start(final QGCUsbSerialHost usbHost, final QGCUsbSerialCallbacks callbacks) {
        synchronized (_lock) {
            _usbHost = usbHost;
            _callbacks = callbacks;
        }
        refreshDevices(true);
    }

    /** Forgets the devices and closes the open ports, without telling C++. */
    void stop() {
        final List<QGCUsbSerialPort> ports;
        synchronized (_lock) {
            ports = new ArrayList<>(_ports.values());
            _ports.clear();
            _devices.clear();
            _usbHost = null;
            _callbacks = _defaultCallbacks;
        }
        for (final QGCUsbSerialPort port : ports) {
            port.close();
        }
    }

    // ------------------------------------------------------------------------
    // USB events and device tracking
    // ------------------------------------------------------------------------

    /**
     * Handles a USB event; {@code device} is null if Android named none. Not for the UI thread: this calls into
     * the USB service, and on a detach it waits for the device's ports to close, which can take seconds. An
     * event still being handled when {@link #stop} runs brings no device back, because none is added without the
     * host.
     */
    void onUsbEvent(final QGCUsbSerialEvent event, final UsbDevice device) {
        boolean changed = false;
        if (device != null) {
            switch (event) {
                // The probe below finds the device.
                case ATTACHED -> {}
                case DETACHED -> {
                    // Device IDs are reused, so the probe below may already find a new device under this one.
                    synchronized (_lock) {
                        changed = _devices.remove(device.getDeviceId()) != null;
                    }
                    disconnectPorts(List.of(device.getDeviceId()));
                }
                case PERMISSION_GRANTED -> changed = true;
                case PERMISSION_DENIED -> {
                    QGCLogger.w(TAG, "USB permission denied for " + device.getDeviceName());
                    synchronized (_lock) {
                        final TrackedDevice tracked = _devices.get(device.getDeviceId());
                        if (tracked != null) {
                            tracked.prompt.denied();
                        }
                    }
                }
            }
        }

        if (refreshDevices(false) || changed) {
            notifyDevicesChanged();
        }
    }

    /** Tells C++ that the user let QGC open a device, unless that is known not to be a serial device. */
    void onDeviceGranted(final UsbDevice device) {
        final boolean mayBeSerial;
        synchronized (_lock) {
            mayBeSerial = device == null || _devices.containsKey(device.getDeviceId());
        }
        if (mayBeSerial) {
            notifyDevicesChanged();
        }
    }

    private void notifyDevicesChanged() {
        final QGCUsbSerialCallbacks callbacks;
        synchronized (_lock) {
            callbacks = _callbacks;
        }
        callbacks.devicesChanged();
    }

    /**
     * Syncs the tracked devices with the attached ones that the library has a driver for.
     *
     * <p>Permission is requested here only at startup, for the devices that are already attached. For a
     * device attached later, Android itself asks the user whether QGC should open it, because the manifest's
     * USB_DEVICE_ATTACHED filter (res/xml/device_filter.xml) lists every device that the prober finds,
     * and accepting grants the permission. Requesting it as well would stack a second dialog on top of
     * that one. If the user dismisses Android's prompt, {@link #open} asks.
     *
     * <p>Returns whether a device was added or removed.
     */
    private boolean refreshDevices(final boolean requestPermission) {
        final List<Integer> detached = new ArrayList<>();
        final boolean devicesAdded;
        synchronized (_refreshLock) {
            final QGCUsbSerialHost usbHost;
            synchronized (_lock) {
                usbHost = _usbHost;
            }
            if (usbHost == null) {
                return false;
            }

            final Map<Integer, UsbSerialDriver> attached = new LinkedHashMap<>();
            for (final UsbSerialDriver driver : usbHost.findDrivers()) {
                attached.put(driver.getDevice().getDeviceId(), driver);
            }

            final List<UsbDevice> added = new ArrayList<>();
            synchronized (_lock) {
                if (_usbHost != usbHost) {
                    return false;
                }
                for (final Integer usbDeviceId : _devices.keySet()) {
                    if (!attached.containsKey(usbDeviceId)) {
                        detached.add(usbDeviceId);
                    }
                }
                _devices.keySet().removeAll(detached);

                for (final UsbSerialDriver driver : attached.values()) {
                    final UsbDevice device = driver.getDevice();
                    if (_devices.putIfAbsent(device.getDeviceId(), new TrackedDevice(driver)) == null) {
                        added.add(device);
                    }
                }
            }

            for (final UsbDevice device : added) {
                QGCLogger.i(TAG, "Added driver for " + device.getDeviceName());
                if (usbHost.hasPermission(device)) {
                    continue;
                }
                if (requestPermission) {
                    requestPermission(usbHost, device);
                } else {
                    QGCLogger.i(TAG, "No permission yet for " + device.getDeviceName());
                }
            }
            devicesAdded = !added.isEmpty();
        }
        // Not under the sync lock: a port can take seconds to close, and C++ is told on this thread.
        disconnectPorts(detached);
        return devicesAdded || !detached.isEmpty();
    }

    /** Closes the ports of the given USB devices and tells C++ which open ones can no longer transfer data. */
    private void disconnectPorts(final List<Integer> usbDeviceIds) {
        final List<QGCUsbSerialPort> ports = new ArrayList<>();
        synchronized (_lock) {
            for (final Iterator<QGCUsbSerialPort> it = _ports.values().iterator(); it.hasNext();) {
                final QGCUsbSerialPort port = it.next();
                if (usbDeviceIds.contains(port.address.usbDeviceId())) {
                    it.remove();
                    ports.add(port);
                }
            }
        }
        for (final QGCUsbSerialPort port : ports) {
            port.closeDisconnected();
        }
    }

    // ------------------------------------------------------------------------
    // Lookup
    // ------------------------------------------------------------------------

    /**
     * Learns which of the tracked devices QGC may open by now, and the serial number of each the first time it
     * may. Both take a call into Android's USB service, so this asks without {@code _lock}, and only about the
     * devices not yet known to be permitted: a permission lasts until its device detaches. What looks a device
     * up or lists the devices does this first.
     */
    private void learnPermissions() {
        final QGCUsbSerialHost usbHost;
        final List<TrackedDevice> undecided = new ArrayList<>();
        synchronized (_lock) {
            usbHost = _usbHost;
            for (final TrackedDevice tracked : _devices.values()) {
                if (!tracked.permitted) {
                    undecided.add(tracked);
                }
            }
        }
        if (usbHost == null) {
            return;
        }
        for (final TrackedDevice tracked : undecided) {
            final UsbDevice device = tracked.device;
            try {
                // Android 10 and later refuse the serial number without permission; Android 9 would hand it out.
                if (!usbHost.hasPermission(device)) {
                    continue;
                }
                final String serialNumber = device.getSerialNumber();
                synchronized (_lock) {
                    tracked.serialNumber = serialNumber;
                    tracked.permitted = true;
                }
            } catch (final SecurityException e) {
                // Some integrated controllers, like the Siyi UNIRC7, use the device for video
                // and deny access to its details without permission. Skip it quietly.
            }
        }
    }

    private boolean hasNameLocked(final TrackedDevice tracked, final QGCUsbSerialPortName portName) {
        final UsbDevice device = tracked.device;
        if (device.getDeviceName().equals(portName.deviceName())) {
            return true;
        }
        // The stable name contains the serial number, which is known only once the device has permission.
        return portName.isStable() && tracked.permitted
                && QGCUsbSerialPortName.stableDeviceName(device, tracked.serialNumber).equals(portName.deviceName());
    }

    /** Returns how many devices have the name. Only a stable name can be that of more than one. */
    private int countDevicesLocked(final QGCUsbSerialPortName portName) {
        int count = 0;
        for (final TrackedDevice tracked : _devices.values()) {
            if (hasNameLocked(tracked, portName)) {
                count++;
            }
        }
        return count;
    }

    /**
     * Returns the only device that has the name, or null. Devices of a kind that report the same
     * serial number, or none, share their stable name, which then stands for none of them: the one found first
     * could be on another vehicle than the one that was meant.
     */
    private TrackedDevice findDeviceLocked(final QGCUsbSerialPortName portName) {
        TrackedDevice found = null;
        for (final TrackedDevice tracked : _devices.values()) {
            if (!hasNameLocked(tracked, portName)) {
                continue;
            }
            if (found != null) {
                return null;
            }
            found = tracked;
        }
        return found;
    }

    /**
     * Returns the device that {@link #open} and {@link #requestPermission} take the name for, or null. A stable
     * name contains the serial number, which Android withholds until the device has permission. One that no
     * device with permission has is therefore taken for the only device without permission that has its vendor
     * and product IDs, if there is exactly one, so that the user gets asked for that device.
     */
    private TrackedDevice findDeviceToOpenLocked(final QGCUsbSerialPortName portName) {
        TrackedDevice found = findDeviceLocked(portName);
        if (found != null || !portName.isStable() || countDevicesLocked(portName) > 1) {
            return found;
        }
        for (final TrackedDevice tracked : _devices.values()) {
            if (tracked.permitted || !portName.hasIdsOf(tracked.device)) {
                continue;
            }
            if (found != null) {
                return null;
            }
            found = tracked;
        }
        return found;
    }

    /** Returns the only device that has the name, if it has the port that the name is of, or else null. */
    private TrackedDevice findDeviceWithPortLocked(final QGCUsbSerialPortName portName) {
        final TrackedDevice tracked = findDeviceLocked(portName);
        return (tracked != null && tracked.hasPort(portName)) ? tracked : null;
    }

    /** Returns the driver's port that the name is of, or null if the driver is null or has no such port. */
    private static UsbSerialPort driverPort(final UsbSerialDriver driver, final QGCUsbSerialPortName portName) {
        if (driver == null) {
            return null;
        }
        // The library numbers a driver's ports by their position in getPorts(), so that position is the
        // index in a port's name and in its PortAddress, and getPortNumber() is never needed.
        final List<UsbSerialPort> ports = driver.getPorts();
        return (portName.portIndex() < ports.size()) ? ports.get(portName.portIndex()) : null;
    }

    private QGCUsbSerialPort findPortLocked(final PortAddress address) {
        for (final QGCUsbSerialPort port : _ports.values()) {
            if (port.address.equals(address)) {
                return port;
            }
        }
        return null;
    }

    private QGCUsbSerialPort registerPortLocked(final PortAddress address, final UsbSerialPort driverPort,
                                                final long handle) {
        final QGCUsbSerialPort port = new QGCUsbSerialPort(_nextPortId, handle, address, driverPort, _callbacks);
        _nextPortId = (_nextPortId == Integer.MAX_VALUE) ? 1 : _nextPortId + 1;
        _ports.put(handle, port);
        return port;
    }

    /** Returns the port with this handle, or null. A port leaves the table when it is closed. */
    QGCUsbSerialPort port(final long handle) {
        return _ports.get(handle);
    }

    /** Returns the handle of the open port with this name, or 0. */
    long portHandle(final String name) {
        final QGCUsbSerialPortName portName = QGCUsbSerialPortName.parse(name);
        learnPermissions();
        synchronized (_lock) {
            final TrackedDevice tracked = findDeviceWithPortLocked(portName);
            if (tracked == null) {
                QGCLogger.w(TAG, "Attempt to get the handle of unknown device " + name);
                return NO_HANDLE;
            }
            final QGCUsbSerialPort port =
                    findPortLocked(new PortAddress(tracked.device.getDeviceId(), portName.portIndex()));
            return (port != null) ? port.handle : NO_HANDLE;
        }
    }

    boolean isPortNameValid(final String name) {
        learnPermissions();
        synchronized (_lock) {
            return findDeviceWithPortLocked(QGCUsbSerialPortName.parse(name)) != null;
        }
    }

    // ------------------------------------------------------------------------
    // Device enumeration and permission
    // ------------------------------------------------------------------------

    /**
     * Returns the ports of the attached devices, also of those that QGC may not open yet. Android withholds the
     * serial number of such a device, so it and the stable name of its ports are empty, by which C++ tells them.
     * A device that stayed hidden until it had permission would stay so for good once the user has dismissed the
     * prompt for it: {@link #open} is what asks again.
     */
    QGCUsbSerialPortInfo[] availablePorts() {
        final List<QGCUsbSerialPortInfo> portInfos = new ArrayList<>();
        learnPermissions();
        synchronized (_lock) {
            for (final TrackedDevice tracked : _devices.values()) {
                final boolean permitted = tracked.permitted;
                final UsbDevice device = tracked.device;
                final int portCount = tracked.portCount;
                final String stableName =
                        permitted ? QGCUsbSerialPortName.stableDeviceName(device, tracked.serialNumber) : null;
                for (int portIndex = 0; portIndex < portCount; portIndex++) {
                    portInfos.add(new QGCUsbSerialPortInfo(
                            new QGCUsbSerialPortName(device.getDeviceName(), portIndex).format(portCount),
                            permitted ? new QGCUsbSerialPortName(stableName, portIndex).format(portCount) : null,
                            device.getProductName(),
                            device.getManufacturerName(),
                            tracked.serialNumber,
                            device.getVendorId(),
                            device.getProductId()));
                }
            }
        }
        return portInfos.toArray(new QGCUsbSerialPortInfo[0]);
    }

    /** Returns whether QGC may open the device of the port with this name. */
    boolean hasPermission(final String name) {
        final QGCUsbSerialPortName portName = QGCUsbSerialPortName.parse(name);
        learnPermissions();
        synchronized (_lock) {
            final TrackedDevice tracked = findDeviceWithPortLocked(portName);
            return tracked != null && tracked.permitted;
        }
    }

    /**
     * Asks the user to let QGC open the device of the port with this name, unless it already may. The user
     * answers later. Unlike {@link #open}, this asks whenever it is called, also after the user has denied the
     * permission. Returns false if there is no such device.
     */
    boolean requestPermission(final String name) {
        final QGCUsbSerialPortName portName = QGCUsbSerialPortName.parse(name);
        final QGCUsbSerialHost usbHost;
        final TrackedDevice tracked;
        final boolean permitted;
        learnPermissions();
        synchronized (_lock) {
            usbHost = _usbHost;
            tracked = findDeviceToOpenLocked(portName);
            if (usbHost == null || tracked == null || !tracked.hasPort(portName)) {
                QGCLogger.w(TAG, "Attempt to request permission for unknown device " + name);
                return false;
            }
            permitted = tracked.permitted;
        }
        if (!permitted) {
            requestPermission(usbHost, tracked.device);
        }
        return true;
    }

    /**
     * Asks the user to let QGC open the device, and notes that it has, so that {@link #open} does not ask for the
     * device as well.
     */
    private void requestPermission(final QGCUsbSerialHost usbHost, final UsbDevice device) {
        synchronized (_lock) {
            final TrackedDevice tracked = _devices.get(device.getDeviceId());
            if (tracked != null) {
                tracked.prompt.asked(_clockMs.getAsLong());
            }
        }
        QGCLogger.i(TAG, "Requesting permission for " + device.getDeviceName());
        usbHost.requestPermission(device);
    }

    /**
     * Refuses to open a port of a device that QGC may not open, and keeps as the failure detail what has become
     * of the permission. Asks the user for it first, who answers later, if the device's
     * {@link QGCUsbSerialPermissionPrompt} calls for it.
     */
    @QGCUsbSerialResult.Code
    private int refuseWithoutPermission(final QGCUsbSerialHost usbHost, final TrackedDevice tracked,
                                        final String name) {
        final boolean canAskUser = usbHost.canAskUser();
        final QGCUsbSerialPermissionPrompt.Refusal refusal;
        synchronized (_lock) {
            refusal = tracked.prompt.refuseOpen(_clockMs.getAsLong(), canAskUser);
        }

        if (refusal.asksUser()) {
            requestPermission(usbHost, tracked.device);
        }
        // C++ tries again for as long as the device is attached, so only the first refusal is a warning.
        final String message = "No USB permission to open " + name + ": " + refusal.detail();
        if (refusal.isFirst()) {
            QGCLogger.w(TAG, message);
        } else {
            QGCLogger.d(TAG, message);
        }
        QGCUsbSerialLastError.set(refusal.detail());
        return QGCUsbSerialResult.PERMISSION_DENIED;
    }

    // ------------------------------------------------------------------------
    // Open / close
    // ------------------------------------------------------------------------

    /**
     * Opens a port under the handle C++ chose for it. Returns the {@link QGCUsbSerialResult} that says how it
     * went. For a device that QGC may not open that is {@code PERMISSION_DENIED}, and the open may ask the user
     * for the permission ({@link #refuseWithoutPermission}).
     */
    @QGCUsbSerialResult.Code
    int open(final String name, final long handle) {
        final QGCUsbSerialPortName portName = QGCUsbSerialPortName.parse(name);
        final QGCUsbSerialHost usbHost;
        final TrackedDevice tracked;
        final QGCUsbSerialPort port;
        learnPermissions();
        synchronized (_lock) {
            if (handle == NO_HANDLE || _ports.containsKey(handle)) {
                QGCLogger.w(TAG, "Attempt to open " + name + " under an unusable handle");
                QGCUsbSerialLastError.set("Unusable port handle " + handle);
                return QGCUsbSerialResult.FAILED;
            }
            usbHost = _usbHost;
            tracked = findDeviceToOpenLocked(portName);
            if (usbHost == null || tracked == null || !tracked.hasPort(portName)) {
                final int namesakes = countDevicesLocked(portName);
                if (namesakes > 1) {
                    QGCLogger.w(TAG, "Attempt to open " + name + ", the name of " + namesakes + " attached devices");
                    QGCUsbSerialLastError.set(namesakes + " attached devices have this name, so it opens none of them");
                    return QGCUsbSerialResult.FAILED;
                }
                QGCLogger.w(TAG, "Attempt to open unknown device " + name);
                return QGCUsbSerialResult.DEVICE_NOT_FOUND;
            }

            final PortAddress address = new PortAddress(tracked.device.getDeviceId(), portName.portIndex());
            if (findPortLocked(address) != null) {
                // Like desktop QSerialPort, refuse instead of silently taking the port from its owner.
                QGCLogger.w(TAG, "Attempt to open " + name + ", which is already open");
                return QGCUsbSerialResult.ALREADY_OPEN;
            }
            if (tracked.permitted) {
                // A driver of the port's own, and so a driver port that no port before had. An I/O thread that
                // outlives the close of its port would call into a driver port shared with the next one; as it
                // is, such a thread only ever reaches a closed one.
                final UsbSerialPort ownDriverPort = driverPort(usbHost.probeDriver(tracked.device), portName);
                if (ownDriverPort == null) {
                    QGCLogger.w(TAG, "No driver to open " + name + " with");
                    QGCUsbSerialLastError.set("The device got no driver");
                    return QGCUsbSerialResult.FAILED;
                }
                // Registered before it is opened, so a second opener is refused while this one blocks on USB
                // transfers.
                port = registerPortLocked(address, ownDriverPort, handle);
            } else {
                // A device that QGC may not open gets no port, which C++ would have it make anew every few seconds.
                port = null;
            }
        }
        if (port == null) {
            return refuseWithoutPermission(usbHost, tracked, name);
        }

        int result = QGCUsbSerialResult.FAILED;
        try {
            result = port.open(usbHost);
        } finally {
            // Also on an unexpected exception: a port left registered would make every later open "already open".
            if (result != QGCUsbSerialResult.OK) {
                synchronized (_lock) {
                    _ports.remove(handle);
                }
                port.close();
            }
        }
        if (result == QGCUsbSerialResult.OK) {
            QGCLogger.i(TAG, "Opened " + name + " as port " + port.id);
        }
        return result;
    }

    /** Closing a port that is already closed succeeds. */
    @QGCUsbSerialResult.Code
    int close(final long handle) {
        final QGCUsbSerialPort port;
        synchronized (_lock) {
            port = _ports.remove(handle);
        }
        if (port == null) {
            QGCLogger.d(TAG, "Close requested for a port that is already closed");
            return QGCUsbSerialResult.OK;
        }
        return port.close() ? QGCUsbSerialResult.OK : QGCUsbSerialResult.IO_ERROR;
    }
}
