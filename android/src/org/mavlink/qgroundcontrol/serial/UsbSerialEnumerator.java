package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Owns the tracked-driver list and all device-name / port-index parsing. Acts as the
 * single source of truth for "which serial drivers are currently attached" — every
 * code path that needs to look up a driver by name, by physical-device id, or by
 * exposed port name routes through this class.
 *
 * <p>All driver-list mutations go through {@link #lock} so that compound operations
 * (probe → removeStale → addNew) are atomic. The {@link StaleDriverCallback} is
 * invoked outside the lock to prevent re-entrancy deadlocks from the manager.</p>
 *
 * <p>Stale-driver cleanup is delegated back to the caller via {@link StaleDriverCallback}
 * because removing a driver also has to release its resource-registry entries, and
 * that registry lives on the manager.</p>
 */
final class UsbSerialEnumerator {

    private static final String TAG = UsbSerialEnumerator.class.getSimpleName();
    static final String PORT_SUFFIX = "#p";

    interface StaleDriverCallback {
        void onPhysicalDeviceRemoved(int physicalDeviceId);
    }

    private final UsbSerialProber prober;
    private final StaleDriverCallback staleCallback;
    private final List<UsbSerialDriver> drivers = new ArrayList<>();
    private final ReentrantLock lock = new ReentrantLock();

    UsbSerialEnumerator(final UsbSerialProber prober, final StaleDriverCallback staleCallback) {
        this.prober = prober;
        this.staleCallback = staleCallback;
    }

    boolean isEmpty() {
        lock.lock();
        try {
            return drivers.isEmpty();
        } finally {
            lock.unlock();
        }
    }

    void clear() {
        lock.lock();
        try {
            drivers.clear();
        } finally {
            lock.unlock();
        }
    }

    UsbSerialDriver findDriverByDeviceId(final int deviceId) {
        lock.lock();
        try {
            for (final UsbSerialDriver driver : drivers) {
                if (driver.getDevice().getDeviceId() == deviceId) {
                    return driver;
                }
            }
            return null;
        } finally {
            lock.unlock();
        }
    }

    UsbSerialDriver findDriverByDeviceName(final String deviceName) {
        lock.lock();
        try {
            for (final UsbSerialDriver driver : drivers) {
                if (driver.getDevice().getDeviceName().equals(deviceName)) {
                    return driver;
                }
            }
            return null;
        } finally {
            lock.unlock();
        }
    }

    /** Returns the freshly-probed driver list without mutating the tracked set. */
    List<UsbSerialDriver> probeAll(final UsbManager usbManager) {
        if (prober == null || usbManager == null) {
            return new ArrayList<>();
        }
        return prober.findAllDrivers(usbManager);
    }

    /** Adds {@code newDriver} unless its physical device is already tracked. */
    void addDriver(final UsbSerialDriver newDriver) {
        final UsbDevice device = newDriver.getDevice();
        lock.lock();
        try {
            for (final UsbSerialDriver d : drivers) {
                if (d.getDevice().getDeviceId() == device.getDeviceId()) {
                    QGCLogger.d(TAG, "Driver already tracked for device ID " + device.getDeviceId());
                    return;
                }
            }
            drivers.add(newDriver);
        } finally {
            lock.unlock();
        }
        QGCLogger.i(TAG, "Adding new driver for device ID " + device.getDeviceId() + ": "
                + device.getDeviceName()
                + String.format(" vid=0x%04x pid=0x%04x", device.getVendorId(), device.getProductId()));
    }

    /**
     * Removes drivers whose physical device is no longer present in {@code currentDrivers},
     * and notifies the {@link StaleDriverCallback} so the caller can release any registry
     * entries tied to that physical device. The callback is invoked outside the lock.
     */
    void removeStaleDrivers(final List<UsbSerialDriver> currentDrivers) {
        final List<Integer> stalePhysicalIds;
        lock.lock();
        try {
            stalePhysicalIds = removeStaleDriversLocked(currentDrivers);
        } finally {
            lock.unlock();
        }
        invokeStaleCallbacks(stalePhysicalIds);
    }

    /** Caller must hold {@link #lock}. Returns the stale physical-device IDs to notify. */
    private List<Integer> removeStaleDriversLocked(final List<UsbSerialDriver> currentDrivers) {
        final List<Integer> stalePhysicalIds = new ArrayList<>();
        drivers.removeIf(existingDriver -> {
            final int existingPhysicalDeviceId = existingDriver.getDevice().getDeviceId();
            final boolean found = currentDrivers.stream()
                    .anyMatch(d -> d.getDevice().getDeviceId() == existingPhysicalDeviceId);
            if (!found) {
                stalePhysicalIds.add(existingPhysicalDeviceId);
                QGCLogger.i(TAG, "Removed stale driver for device ID " + existingPhysicalDeviceId);
                return true;
            }
            return false;
        });
        return stalePhysicalIds;
    }

    private void invokeStaleCallbacks(final List<Integer> stalePhysicalIds) {
        for (final int physicalDeviceId : stalePhysicalIds) {
            staleCallback.onPhysicalDeviceRemoved(physicalDeviceId);
        }
    }

    void addNewDrivers(final List<UsbSerialDriver> currentDrivers) {
        lock.lock();
        try {
            addNewDriversLocked(currentDrivers);
        } finally {
            lock.unlock();
        }
    }

    /** Caller must hold {@link #lock}. */
    private void addNewDriversLocked(final List<UsbSerialDriver> currentDrivers) {
        for (final UsbSerialDriver newDriver : currentDrivers) {
            final int deviceId = newDriver.getDevice().getDeviceId();
            boolean alreadyTracked = false;
            for (final UsbSerialDriver d : drivers) {
                if (d.getDevice().getDeviceId() == deviceId) {
                    alreadyTracked = true;
                    break;
                }
            }
            if (alreadyTracked) {
                QGCLogger.d(TAG, "Driver already tracked for device ID " + deviceId);
                continue;
            }
            drivers.add(newDriver);
            final UsbDevice dev = newDriver.getDevice();
            QGCLogger.i(TAG, "Adding new driver for device ID " + deviceId + ": "
                    + dev.getDeviceName()
                    + String.format(" vid=0x%04x pid=0x%04x", dev.getVendorId(), dev.getProductId()));
        }
    }

    void updateCurrentDrivers(final UsbManager usbManager) {
        if (prober == null || usbManager == null) {
            QGCLogger.w(TAG, "USB serial enumerator not ready, skipping driver refresh");
            return;
        }
        final List<UsbSerialDriver> currentDrivers = probeAll(usbManager);
        // Single lock acquisition makes remove-stale + add-new atomic; callbacks outside lock to prevent re-entrancy.
        final List<Integer> stalePhysicalIds;
        lock.lock();
        try {
            stalePhysicalIds = removeStaleDriversLocked(currentDrivers);
            if (!currentDrivers.isEmpty()) {
                addNewDriversLocked(currentDrivers);
            }
        } finally {
            lock.unlock();
        }
        invokeStaleCallbacks(stalePhysicalIds);
    }

    UsbPortInfo[] availablePortsInfo() {
        // Snapshot under lock; UsbDevice introspection (productName, serialNumber) can throw
        // SecurityException and must not run with the lock held.
        final List<UsbSerialDriver> snapshot;
        lock.lock();
        try {
            if (drivers.isEmpty()) {
                return new UsbPortInfo[0];
            }
            snapshot = new ArrayList<>(drivers);
        } finally {
            lock.unlock();
        }

        final List<UsbPortInfo> portInfoList = new ArrayList<>();
        for (final UsbSerialDriver driver : snapshot) {
            try {
                final List<UsbSerialPort> ports = driver.getPorts();
                if (ports == null || ports.isEmpty()) {
                    continue;
                }
                final UsbDevice device = driver.getDevice();
                final int portCount = ports.size();
                for (final UsbSerialPort port : ports) {
                    final int portIndex = port.getPortNumber();
                    final String exposedDeviceName = buildPortDeviceName(device, portIndex, portCount);
                    final UsbPortInfo info = new UsbPortInfo(
                            exposedDeviceName,
                            Objects.toString(device.getProductName(), ""),
                            Objects.toString(device.getManufacturerName(), ""),
                            Objects.toString(device.getSerialNumber(), ""),
                            device.getProductId(),
                            device.getVendorId());
                    portInfoList.add(info);
                }
            } catch (final SecurityException e) {
                // Some integrated controllers (e.g. Siyi UNIRC7) expose a USB device for video
                // output. Accessing device info without permission throws SecurityException;
                // swallow it to avoid log spam.
            }
        }
        return portInfoList.toArray(new UsbPortInfo[0]);
    }

    static final class DevicePortSpec {
        final String baseDeviceName;
        final int portIndex;

        DevicePortSpec(final String baseDeviceName, final int portIndex) {
            this.baseDeviceName = baseDeviceName;
            this.portIndex = Math.max(0, portIndex);
        }
    }

    static DevicePortSpec parseDevicePortSpec(final String deviceName) {
        if (deviceName == null || deviceName.isEmpty()) {
            return new DevicePortSpec("", 0);
        }
        final int split = deviceName.lastIndexOf(PORT_SUFFIX);
        if (split <= 0) {
            return new DevicePortSpec(deviceName, 0);
        }
        final String baseName = deviceName.substring(0, split);
        final String suffix = deviceName.substring(split + PORT_SUFFIX.length());
        try {
            final int parsedIndex = Integer.parseInt(suffix);
            return new DevicePortSpec(baseName, Math.max(0, parsedIndex));
        } catch (final NumberFormatException e) {
            QGCLogger.w(TAG, "Invalid device port suffix in " + deviceName + ", defaulting to port 0");
            return new DevicePortSpec(deviceName, 0);
        }
    }

    static String buildPortDeviceName(final UsbDevice device, final int portIndex, final int portCount) {
        final String baseName = device.getDeviceName();
        if (portCount <= 1 && portIndex == 0) {
            return baseName;
        }
        return baseName + PORT_SUFFIX + portIndex;
    }

    static UsbSerialPort getPortFromDriver(final UsbSerialDriver driver, final int portIndex) {
        if (driver == null) {
            return null;
        }
        final List<UsbSerialPort> ports = driver.getPorts();
        if (ports == null || ports.isEmpty()) {
            return null;
        }
        for (final UsbSerialPort port : ports) {
            if (port.getPortNumber() == portIndex) {
                return port;
            }
        }
        if (portIndex >= 0 && portIndex < ports.size()) {
            return ports.get(portIndex);
        }
        return null;
    }

}
