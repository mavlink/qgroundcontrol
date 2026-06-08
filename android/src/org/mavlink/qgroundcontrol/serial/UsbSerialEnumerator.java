package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import androidx.annotation.AnyThread;
import androidx.annotation.GuardedBy;

import com.hoho.android.usbserial.driver.ProlificSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * Single source of truth for which serial drivers are currently attached; owns the tracked-driver list and all device-name / port-index parsing.
 *
 * <p>All driver-list mutations go through {@link #lock} so compound operations (probe → removeStale → addNew) are atomic. {@link StaleDriverCallback}
 * is invoked outside the lock (it releases manager-side registry entries) to prevent re-entrancy deadlocks.</p>
 */
final class UsbSerialEnumerator {

    private static final String TAG = UsbSerialEnumerator.class.getSimpleName();
    static final String PORT_SUFFIX = "#p";

    interface StaleDriverCallback {
        void onPhysicalDeviceRemoved(String deviceName);
    }

    private final UsbSerialProber prober;
    private volatile StaleDriverCallback staleCallback;
    @GuardedBy("lock")
    private final List<UsbSerialDriver> drivers = new ArrayList<>();
    private final Object lock = new Object();

    UsbSerialEnumerator(final UsbSerialProber prober) {
        this.prober = prober;
    }

    /** Bound post-construction to break the enumerator↔lifecycle ctor cycle; must be called before any {@code removeStale}. */
    void setStaleDriverCallback(final StaleDriverCallback callback) {
        this.staleCallback = callback;
    }

    void clear() {
        synchronized (lock) {
            drivers.clear();
        }
    }

    UsbSerialDriver findDriverByDeviceName(final String deviceName) {
        synchronized (lock) {
            for (final UsbSerialDriver driver : drivers) {
                if (driver.getDevice().getDeviceName().equals(deviceName)) {
                    return driver;
                }
            }
            return null;
        }
    }

    /** Returns the freshly-probed driver list without mutating the tracked set. */
    private List<UsbSerialDriver> probeAll(final UsbManager usbManager) {
        if (prober == null || usbManager == null) {
            return new ArrayList<>();
        }
        return prober.findAllDrivers(usbManager);
    }

    /** Caller must hold {@link #lock}. Returns true if the driver was added. */
    private boolean addDriverLocked(final UsbSerialDriver newDriver) {
        final UsbDevice device = newDriver.getDevice();
        final String deviceName = device.getDeviceName();
        for (final UsbSerialDriver d : drivers) {
            if (d.getDevice().getDeviceName().equals(deviceName)) {
                QGCLogger.d(TAG, "Driver already tracked for device " + deviceName);
                return false;
            }
        }
        drivers.add(newDriver);
        QGCLogger.i(TAG, "Adding new driver for device " + deviceName
                + String.format(" vid=0x%04x pid=0x%04x", device.getVendorId(), device.getProductId()));
        return true;
    }

    /** Caller must hold {@link #lock}. Returns the stale device names to notify. */
    private List<String> removeStaleDriversLocked(final List<UsbSerialDriver> currentDrivers) {
        final List<String> staleDeviceNames = new ArrayList<>();
        drivers.removeIf(existingDriver -> {
            final String existingDeviceName = existingDriver.getDevice().getDeviceName();
            final boolean found = currentDrivers.stream()
                    .anyMatch(d -> d.getDevice().getDeviceName().equals(existingDeviceName));
            if (!found) {
                staleDeviceNames.add(existingDeviceName);
                QGCLogger.i(TAG, "Removed stale driver for device " + existingDeviceName);
                return true;
            }
            return false;
        });
        return staleDeviceNames;
    }

    private void invokeStaleCallbacks(final List<String> staleDeviceNames) {
        final StaleDriverCallback cb = staleCallback;
        if (cb == null) return;
        for (final String deviceName : staleDeviceNames) {
            cb.onPhysicalDeviceRemoved(deviceName);
        }
    }

    /** Caller must hold {@link #lock}. */
    private void addNewDriversLocked(final List<UsbSerialDriver> currentDrivers) {
        for (final UsbSerialDriver newDriver : currentDrivers) {
            addDriverLocked(newDriver);
        }
    }

    void updateCurrentDrivers(final UsbManager usbManager) {
        if (prober == null || usbManager == null) {
            QGCLogger.w(TAG, "USB serial enumerator not ready, skipping driver refresh");
            return;
        }
        final List<UsbSerialDriver> currentDrivers = probeAll(usbManager);
        // Single lock acquisition makes remove-stale + add-new atomic; callbacks outside lock to prevent re-entrancy.
        final List<String> staleDeviceNames;
        synchronized (lock) {
            staleDeviceNames = removeStaleDriversLocked(currentDrivers);
            if (!currentDrivers.isEmpty()) {
                addNewDriversLocked(currentDrivers);
            }
        }
        invokeStaleCallbacks(staleDeviceNames);
    }

    void replaceDriverForDeviceName(final UsbManager usbManager, final String deviceName) {
        if (prober == null || usbManager == null || deviceName == null || deviceName.isEmpty()) {
            return;
        }
        // Drop the stale entry FIRST so a concurrent findDriverByDeviceName() during the unlocked probe returns null (the truthful "being replaced" state) rather than a torn-down driver.
        // Holding lock across probeAll() would stall every other caller for the probe's duration — the reason probeAll runs unlocked.
        synchronized (lock) {
            drivers.removeIf(driver -> driver.getDevice().getDeviceName().equals(deviceName));
        }
        final List<UsbSerialDriver> currentDrivers = probeAll(usbManager);
        synchronized (lock) {
            for (final UsbSerialDriver driver : currentDrivers) {
                if (driver.getDevice().getDeviceName().equals(deviceName)) {
                    addDriverLocked(driver);
                    return;
                }
            }
        }
    }

    @AnyThread
    UsbPortInfo[] availablePortsInfo() {
        // Snapshot under lock; UsbDevice introspection (productName, serialNumber) can throw SecurityException and must not run with the lock held.
        final List<UsbSerialDriver> snapshot;
        synchronized (lock) {
            if (drivers.isEmpty()) {
                return new UsbPortInfo[0];
            }
            snapshot = new ArrayList<>(drivers);
        }

        final List<UsbPortInfo> portInfoList = new ArrayList<>();
        for (final UsbSerialDriver driver : snapshot) {
            final List<UsbSerialPort> ports = driver.getPorts();
            if (ports == null || ports.isEmpty()) {
                continue;
            }
            final UsbDevice device = driver.getDevice();

            // Device introspection can throw SecurityException for permission-less devices (e.g. Siyi UNIRC7 video); read it ONCE up front so a throw skips only this driver, not the rest of a multi-port device.
            final String productName;
            final String manufacturerName;
            final String serialNumber;
            final int productId;
            final int vendorId;
            final boolean prolificLegacyBaudLimited;
            try {
                productName = Objects.toString(device.getProductName(), "");
                manufacturerName = Objects.toString(device.getManufacturerName(), "");
                serialNumber = Objects.toString(device.getSerialNumber(), "");
                productId = device.getProductId();
                vendorId = device.getVendorId();
                prolificLegacyBaudLimited = driver instanceof ProlificSerialDriver
                        && DriverStrategy.isProlificLegacyBaudLimitedDeviceClass(
                                device.getDeviceClass(), device.getDeviceSubclass());
            } catch (final SecurityException e) {
                continue;
            }

            final int portCount = ports.size();
            final DriverStrategy.Kind kind = DriverStrategy.of(driver);
            for (final UsbSerialPort port : ports) {
                final int portIndex = port.getPortNumber();
                portInfoList.add(new UsbPortInfo(
                        buildPortDeviceName(device, portIndex, portCount),
                        productName,
                        manufacturerName,
                        serialNumber,
                        productId,
                        vendorId,
                        DriverStrategy.supportedBaudRates(kind, prolificLegacyBaudLimited)));
            }
        }
        return portInfoList.toArray(new UsbPortInfo[0]);
    }

    /** Immutable (baseDeviceName, portIndex) pair. Construct via {@link #of} so portIndex is clamped &ge; 0. */
    record DevicePortSpec(String baseDeviceName, int portIndex) {
        static DevicePortSpec of(final String baseDeviceName, final int portIndex) {
            return new DevicePortSpec(baseDeviceName, Math.max(0, portIndex));
        }
    }

    static DevicePortSpec parseDevicePortSpec(final String deviceName) {
        if (deviceName == null || deviceName.isEmpty()) {
            return DevicePortSpec.of("", 0);
        }
        final int split = deviceName.lastIndexOf(PORT_SUFFIX);
        if (split <= 0) {
            return DevicePortSpec.of(deviceName, 0);
        }
        final String baseName = deviceName.substring(0, split);
        final String suffix = deviceName.substring(split + PORT_SUFFIX.length());
        try {
            return DevicePortSpec.of(baseName, Integer.parseInt(suffix));
        } catch (final NumberFormatException e) {
            QGCLogger.w(TAG, "Invalid device port suffix in " + deviceName + ", defaulting to port 0");
            return DevicePortSpec.of(deviceName, 0);
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
        QGCLogger.w(TAG, "No port with index " + portIndex + " on driver " + driver.getClass().getSimpleName());
        return null;
    }

}
