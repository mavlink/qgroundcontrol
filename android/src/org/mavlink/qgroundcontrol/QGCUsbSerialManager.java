package org.mavlink.qgroundcontrol;

import android.app.PendingIntent;
import android.content.*;
import android.hardware.usb.*;
import android.os.Build;
import android.os.Process;
import com.hoho.android.usbserial.driver.*;
import com.hoho.android.usbserial.util.*;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class QGCUsbSerialManager {
    private static final String TAG = QGCUsbSerialManager.class.getSimpleName();
    private static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    // Sentinel values: BAD_DEVICE_ID (0) for invalid device IDs,
    // getDeviceHandle() returns -1 for missing file descriptors.
    private static final int BAD_DEVICE_ID = 0;
    private static final int READ_BUF_SIZE = 2048;
    private static final int MAX_NATIVE_CALLBACK_DATA_BYTES = 16 * 1024;
    private static final String PORT_SUFFIX = "#p";
    private static final Object lifecycleLock = new Object();

    private static UsbManager usbManager;
    private static Context appContext;
    private static PendingIntent usbPermissionIntent;
    private static UsbSerialProber usbSerialProber;
    private static boolean receiverRegistered;

    private static final List<UsbSerialDriver> drivers = new CopyOnWriteArrayList<>();
    private static final ConcurrentHashMap<Integer, UsbDeviceResources> deviceResourcesMap = new ConcurrentHashMap<>();
    private static final ConcurrentHashMap<PortAddress, Integer> portAddressToResourceId = new ConcurrentHashMap<>();
    private static final ConcurrentHashMap<Integer, PortAddress> resourceIdToPortAddress = new ConcurrentHashMap<>();
    private static final AtomicInteger nextResourceId = new AtomicInteger(1);

    interface NativeCallbacks {
        void onDeviceHasDisconnected(long classPtr);
        void onDeviceException(long classPtr, String message);
        void onDeviceNewData(long classPtr, byte[] data);
    }

    private static final class JniNativeCallbacks implements NativeCallbacks {
        @Override
        public void onDeviceHasDisconnected(final long classPtr) {
            nativeDeviceHasDisconnected(classPtr);
        }

        @Override
        public void onDeviceException(final long classPtr, final String message) {
            nativeDeviceException(classPtr, message);
        }

        @Override
        public void onDeviceNewData(final long classPtr, final byte[] data) {
            nativeDeviceNewData(classPtr, data);
        }
    }

    private static volatile NativeCallbacks nativeCallbacks = new JniNativeCallbacks();

    // Native methods
    private static native void nativeDeviceHasDisconnected(final long classPtr);
    private static native void nativeDeviceException(final long classPtr, final String message);
    private static native void nativeDeviceNewData(final long classPtr, final byte[] data);

    static void setNativeCallbacksForTesting(final NativeCallbacks callbacks) {
        nativeCallbacks = (callbacks != null) ? callbacks : new JniNativeCallbacks();
    }

    private static void emitDeviceHasDisconnected(final long classPtr) {
        if (classPtr != 0) {
            nativeCallbacks.onDeviceHasDisconnected(classPtr);
        }
    }

    private static void emitDeviceException(final long classPtr, final String message) {
        if (classPtr != 0) {
            nativeCallbacks.onDeviceException(classPtr, message);
        }
    }

    private static void emitDeviceNewData(final long classPtr, final byte[] data) {
        if (classPtr != 0 && data != null && data.length > 0) {
            nativeCallbacks.onDeviceNewData(classPtr, data);
        }
    }

    @SuppressWarnings("deprecation")
    private static UsbDevice getUsbDevice(Intent intent) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
        }
        return intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
    }

    private static DevicePortSpec parseDevicePortSpec(final String deviceName) {
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

    private static String buildPortDeviceName(final UsbDevice device, final int portIndex, final int portCount) {
        final String baseName = device.getDeviceName();
        if (portCount <= 1 && portIndex == 0) {
            return baseName;
        }
        return baseName + PORT_SUFFIX + portIndex;
    }

    private static UsbSerialPort getPortFromDriver(final UsbSerialDriver driver, final int portIndex) {
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

    private static int getOrCreateResourceId(final int physicalDeviceId, final int portIndex) {
        final PortAddress address = new PortAddress(physicalDeviceId, portIndex);
        final Integer existing = portAddressToResourceId.get(address);
        if (existing != null) {
            return existing;
        }

        final int candidateId = nextResourceId.getAndIncrement();
        final Integer raced = portAddressToResourceId.putIfAbsent(address, candidateId);
        final int resourceId = (raced != null) ? raced : candidateId;
        if (raced == null) {
            resourceIdToPortAddress.put(resourceId, address);
        }
        return resourceId;
    }

    private static void removeResourceMapping(final int resourceId) {
        final PortAddress address = resourceIdToPortAddress.remove(resourceId);
        if (address != null) {
            portAddressToResourceId.remove(address);
        }
    }

    private static List<Integer> findResourceIdsForPhysicalDevice(final int physicalDeviceId) {
        final List<Integer> ids = new ArrayList<>();
        for (Map.Entry<Integer, PortAddress> entry : resourceIdToPortAddress.entrySet()) {
            if (entry.getValue().physicalDeviceId == physicalDeviceId) {
                ids.add(entry.getKey());
            }
        }
        return ids;
    }

    private static void removeAllResourcesForPhysicalDevice(final int physicalDeviceId) {
        for (final Integer resourceId : findResourceIdsForPhysicalDevice(physicalDeviceId)) {
            close(resourceId);
        }
    }

    /**
     * Encapsulates all resources associated with a USB device.
     */
    private static class UsbDeviceResources {
        UsbSerialDriver driver;
        SerialInputOutputManager ioManager;
        int fileDescriptor;
        long classPtr;
        int portIndex;
        int physicalDeviceId;
        String baseDeviceName;

        UsbDeviceResources(final UsbSerialDriver driver, final int portIndex) {
            this.driver = driver;
            this.portIndex = portIndex;
            if (driver != null && driver.getDevice() != null) {
                this.physicalDeviceId = driver.getDevice().getDeviceId();
                this.baseDeviceName = driver.getDevice().getDeviceName();
            }
        }
    }

    private static final class PortAddress {
        final int physicalDeviceId;
        final int portIndex;

        PortAddress(final int physicalDeviceId, final int portIndex) {
            this.physicalDeviceId = physicalDeviceId;
            this.portIndex = portIndex;
        }

        @Override
        public boolean equals(final Object other) {
            if (this == other) {
                return true;
            }
            if (!(other instanceof PortAddress)) {
                return false;
            }
            final PortAddress rhs = (PortAddress) other;
            return (physicalDeviceId == rhs.physicalDeviceId) && (portIndex == rhs.portIndex);
        }

        @Override
        public int hashCode() {
            return Objects.hash(physicalDeviceId, portIndex);
        }
    }

    private static final class DevicePortSpec {
        final String baseDeviceName;
        final int portIndex;

        DevicePortSpec(final String baseDeviceName, final int portIndex) {
            this.baseDeviceName = baseDeviceName;
            this.portIndex = Math.max(0, portIndex);
        }
    }

    /**
     * Initializes the UsbSerialManager. Should be called once, typically from QGCActivity.onCreate().
     *
     * @param context The application context.
     */
    public static void initialize(Context context) {
        synchronized (lifecycleLock) {
            if (usbManager != null) {
                return;
            }

            appContext = context.getApplicationContext();
            usbManager = (UsbManager) appContext.getSystemService(Context.USB_SERVICE);
            if (usbManager == null) {
                QGCLogger.e(TAG, "Failed to get UsbManager");
                return;
            }

            QGCFtdiDriver.initialize(appContext);
            setupUsbPermissionIntent(appContext);
            usbSerialProber = QGCUsbSerialProber.getQGCUsbSerialProber();
            registerUsbReceiver(appContext);
            updateCurrentDrivers();
        }
    }

    /**
     * Cleans up resources by unregistering the BroadcastReceiver.
     * Should be called when the manager is no longer needed, typically from QGCActivity.onDestroy().
     */
    public static void cleanup(Context context) {
        synchronized (lifecycleLock) {
            for (Integer deviceId : new ArrayList<>(deviceResourcesMap.keySet())) {
                close(deviceId);
            }

            final Context unregisterContext = (appContext != null) ? appContext : context;
            try {
                if (receiverRegistered) {
                    unregisterContext.unregisterReceiver(usbReceiver);
                    receiverRegistered = false;
                    QGCLogger.i(TAG, "BroadcastReceiver unregistered successfully.");
                }
            } catch (final IllegalArgumentException e) {
                QGCLogger.w(TAG, "Receiver not registered: " + e.getMessage());
            }

            usbPermissionIntent = null;
            usbSerialProber = null;
            QGCFtdiDriver.cleanup();
            usbManager = null;
            appContext = null;
            drivers.clear();
            deviceResourcesMap.clear();
            portAddressToResourceId.clear();
            resourceIdToPortAddress.clear();
            nextResourceId.set(1);
        }
    }

    /**
     * Sets up the PendingIntent for USB permission requests.
     *
     * @param context The application context.
     */
    private static void setupUsbPermissionIntent(Context context) {
        Intent permissionIntent = new Intent(ACTION_USB_PERMISSION).setPackage(context.getPackageName());
        int flags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            flags |= PendingIntent.FLAG_IMMUTABLE;
        }
        usbPermissionIntent = PendingIntent.getBroadcast(context, 0, permissionIntent, flags);
    }

    /**
     * Registers the BroadcastReceiver to listen for USB-related events.
     *
     * @param context The application context.
     */
    private static void registerUsbReceiver(Context context) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);

        try {
            if (android.os.Build.VERSION.SDK_INT >=
                android.os.Build.VERSION_CODES.TIRAMISU) {
                int flags = Context.RECEIVER_NOT_EXPORTED;
                context.registerReceiver(usbReceiver, filter, flags);
            } else {
                context.registerReceiver(usbReceiver, filter);
            }

            receiverRegistered = true;
            QGCLogger.i(TAG, "BroadcastReceiver registered successfully.");
        } catch (Exception e) {
            receiverRegistered = false;
            QGCLogger.e(TAG, "Failed to register BroadcastReceiver", e);
        }
    }

    /**
     * BroadcastReceiver to handle USB events.
     */
    private static final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            QGCLogger.i(TAG, "BroadcastReceiver USB action " + action);

            switch (action) {
                case ACTION_USB_PERMISSION:
                    handleUsbPermission(intent);
                    break;
                case UsbManager.ACTION_USB_DEVICE_DETACHED:
                    handleUsbDeviceDetached(intent);
                    break;
                case UsbManager.ACTION_USB_DEVICE_ATTACHED:
                    handleUsbDeviceAttached(intent);
                    break;
                default:
                    break;
            }

            updateCurrentDrivers();
        }
    };

    /**
     * Handles USB permission results.
     *
     * @param intent The intent containing permission data.
     */
    private static void handleUsbPermission(final Intent intent) {
        UsbDevice device = getUsbDevice(intent);
        if (device != null) {
            final int physicalDeviceId = device.getDeviceId();
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                QGCLogger.i(TAG, "Permission granted to " + device.getDeviceName());
                addOrUpdateDevice(device);
            } else {
                QGCLogger.i(TAG, "Permission denied for " + device.getDeviceName());
                for (final Integer resourceId : findResourceIdsForPhysicalDevice(physicalDeviceId)) {
                    final UsbDeviceResources resources = deviceResourcesMap.get(resourceId);
                    if (resources != null) {
                        emitDeviceException(resources.classPtr, "USB Permission Denied");
                    }
                }
            }
        }
    }

    /**
     * Handles USB device detachment events.
     *
     * @param intent The intent containing device data.
     */
    private static void handleUsbDeviceDetached(final Intent intent) {
        UsbDevice device = getUsbDevice(intent);
        if (device != null) {
            final int physicalDeviceId = device.getDeviceId();
            final List<Integer> resourceIds = findResourceIdsForPhysicalDevice(physicalDeviceId);
            for (final Integer resourceId : resourceIds) {
                final UsbDeviceResources resources = deviceResourcesMap.get(resourceId);
                if (resources == null) {
                    continue;
                }
                final long classPtr = resources.classPtr;
                close(resourceId);
                emitDeviceHasDisconnected(classPtr);
            }
            QGCLogger.i(TAG, "Device detached: " + device.getDeviceName());
        }
    }

    /**
     * Handles USB device attachment events.
     *
     * @param intent The intent containing device data.
     */
    private static void handleUsbDeviceAttached(final Intent intent) {
        UsbDevice device = getUsbDevice(intent);
        if (device != null) {
            addOrUpdateDevice(device);
        }
    }

    /**
     * Adds a new device or updates an existing one.
     *
     * @param device The UsbDevice to add or update.
     */
    private static void addOrUpdateDevice(UsbDevice device) {
        UsbSerialDriver driver = findDriverByDeviceId(device.getDeviceId());
        if (driver != null) {
            if (usbManager.hasPermission(device)) {
                QGCLogger.i(TAG, "Already have permission to use device " + device.getDeviceName());
                addDriver(driver);
            } else {
                QGCLogger.i(TAG, "Requesting permission to use device " + device.getDeviceName());
                usbManager.requestPermission(device, usbPermissionIntent);
            }
        }
    }

    /**
     * Checks if a device name is valid (i.e., exists in the current driver list).
     *
     * @param name The device name to check.
     * @return True if valid, false otherwise.
     */
    public static boolean isDeviceNameValid(final String name) {
        final DevicePortSpec spec = parseDevicePortSpec(name);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        return getPortFromDriver(driver, spec.portIndex) != null;
    }

    /**
     * Checks if a device name is currently open.
     *
     * @param name The device name to check.
     * @return True if open, false otherwise.
     */
    public static boolean isDeviceNameOpen(final String name) {
        final DevicePortSpec spec = parseDevicePortSpec(name);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            return false;
        }

        final UsbSerialPort port = getPortFromDriver(driver, spec.portIndex);
        return (port != null && port.isOpen());
    }

    /**
     * Retrieves the device ID for a given device name.
     *
     * @param deviceName The device name.
     * @return The device ID, or BAD_DEVICE_ID if not found.
     */
    public static int getDeviceId(final String deviceName) {
        final DevicePortSpec spec = parseDevicePortSpec(deviceName);
        UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to get ID of unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        if (getPortFromDriver(driver, spec.portIndex) == null) {
            QGCLogger.w(TAG, "Attempt to get ID of unknown port index " + spec.portIndex + " for " + spec.baseDeviceName);
            return BAD_DEVICE_ID;
        }

        return getOrCreateResourceId(driver.getDevice().getDeviceId(), spec.portIndex);
    }

    /**
     * Retrieves the native device handle (file descriptor).
     *
     * @param deviceId The device ID.
     * @return The device handle, or -1 if not found.
     */
    public static int getDeviceHandle(final int deviceId) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        return (resources != null) ? resources.fileDescriptor : -1;
    }

    /**
     * Updates the current list of USB serial drivers by scanning connected devices.
     */
    private static void updateCurrentDrivers() {
        if (usbManager == null || usbSerialProber == null) {
            QGCLogger.w(TAG, "USB serial manager not ready, skipping driver refresh");
            return;
        }

        final List<UsbSerialDriver> currentDrivers = usbSerialProber.findAllDrivers(usbManager);
        removeStaleDrivers(currentDrivers);
        if (!currentDrivers.isEmpty()) {
            addNewDrivers(currentDrivers);
        }
    }

    /**
     * Removes drivers that are no longer connected.
     *
     * @param currentDrivers The list of currently connected drivers.
     */
    private static void removeStaleDrivers(final List<UsbSerialDriver> currentDrivers) {
        drivers.removeIf(existingDriver -> {
            final int existingPhysicalDeviceId = existingDriver.getDevice().getDeviceId();
            boolean found = currentDrivers.stream()
                    .anyMatch(currentDriver -> currentDriver.getDevice().getDeviceId() == existingDriver.getDevice().getDeviceId());

            if (!found) {
                removeAllResourcesForPhysicalDevice(existingPhysicalDeviceId);
                QGCLogger.i(TAG, "Removed stale driver for device ID " + existingPhysicalDeviceId);
                return true;
            }
            return false;
        });
    }

    /**
     * Adds new drivers that are not already in the driver list.
     *
     * @param currentDrivers The list of currently connected drivers.
     */
    private static void addNewDrivers(final List<UsbSerialDriver> currentDrivers) {
        for (UsbSerialDriver newDriver : currentDrivers) {
            boolean found = drivers.stream()
                    .anyMatch(existingDriver -> existingDriver.getDevice().getDeviceId() == newDriver.getDevice().getDeviceId());

            if (!found) {
                addDriver(newDriver);
            }
        }
    }

    /**
     * Adds a new USB serial driver to the driver list and requests permission if needed.
     *
     * @param newDriver The UsbSerialDriver to add.
     */
    private static void addDriver(final UsbSerialDriver newDriver) {
        UsbDevice device = newDriver.getDevice();
        String deviceName = device.getDeviceName();

        final boolean alreadyTracked = drivers.stream()
            .anyMatch(existingDriver -> existingDriver.getDevice().getDeviceId() == device.getDeviceId());
        if (alreadyTracked) {
            QGCLogger.d(TAG, "Driver already tracked for device ID " + device.getDeviceId());
            return;
        }

        drivers.add(newDriver);
        QGCLogger.i(TAG, "Adding new driver for device ID " + device.getDeviceId() + ": " + deviceName);

        if (usbManager.hasPermission(device)) {
            QGCLogger.i(TAG, "Already have permission to use device " + deviceName);
        } else {
            QGCLogger.i(TAG, "Requesting permission to use device " + deviceName);
            usbManager.requestPermission(device, usbPermissionIntent);
        }
    }

    /**
     * Finds a USB serial driver by its device ID.
     *
     * @param deviceId The device ID.
     * @return The corresponding UsbSerialDriver or null if not found.
     */
    private static UsbSerialDriver findDriverByDeviceId(final int deviceId) {
        for (UsbSerialDriver driver : drivers) {
            if (driver.getDevice().getDeviceId() == deviceId) {
                return driver;
            }
        }
        return null;
    }

    /**
     * Finds a USB serial driver by its device name.
     *
     * @param deviceName The device name.
     * @return The corresponding UsbSerialDriver or null if not found.
     */
    private static UsbSerialDriver findDriverByDeviceName(final String deviceName) {
        for (UsbSerialDriver driver : drivers) {
            if (driver.getDevice().getDeviceName().equals(deviceName)) {
                return driver;
            }
        }
        return null;
    }

    /**
     * Finds a USB serial port by its device ID.
     *
     * @param deviceId The device ID.
     * @return The corresponding UsbSerialPort or null if not found.
     */
    private static UsbSerialPort findPortByDeviceId(final int deviceId) {
        if (deviceId == BAD_DEVICE_ID) {
            QGCLogger.w(TAG, "Finding port failed for invalid Device ID " + deviceId);
            return null;
        }

        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.driver == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return null;
        }

        UsbSerialPort port = getPortFromDriver(resources.driver, resources.portIndex);
        if (port == null) {
            QGCLogger.w(TAG, "No port available on device ID " + deviceId + " at port index " + resources.portIndex);
            return null;
        }

        return port;
    }

    private static UsbSerialPort getPortOrWarn(final int deviceId, final String operation) {
        final UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to " + operation + " on a null port for device ID " + deviceId);
            return null;
        }

        return port;
    }

    private static UsbSerialPort getOpenPortOrWarn(final int deviceId, final String operation) {
        final UsbSerialPort port = getPortOrWarn(deviceId, operation);
        if (port == null) {
            return null;
        }

        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to " + operation + " on a closed port for device ID " + deviceId);
            return null;
        }

        return port;
    }

    /**
     * Retrieves information about all available USB serial devices.
     *
     * @return An array of device information strings or null if no devices are available.
     */
    public static String[] availableDevicesInfo() {
        if (usbManager == null || drivers.isEmpty()) {
            return null;
        }

        final List<String> deviceInfoList = new ArrayList<>();

        for (final UsbSerialDriver driver : drivers) {
            try {
                final List<UsbSerialPort> ports = driver.getPorts();
                if (ports == null || ports.isEmpty()) {
                    continue;
                }

                final int portCount = ports.size();
                for (final UsbSerialPort port : ports) {
                    final int portIndex = port.getPortNumber();
                    final String exposedDeviceName = buildPortDeviceName(driver.getDevice(), portIndex, portCount);
                    final String deviceInfo = formatDeviceInfo(driver.getDevice(), exposedDeviceName);
                    deviceInfoList.add(deviceInfo);
                }
            } catch (SecurityException e) {
                // On some integrated controllers like the Siyi UNIRC7 the usb device is used for video output.
                // This in turn causes a security exception when trying to access device info without permission.
                // We just eat the exception in these cases to prevent log spamming.
            }
        }

        return deviceInfoList.isEmpty() ? null : deviceInfoList.toArray(new String[0]);
    }

    /**
     * Formats device information into a standardized string.
     *
     * @param device The UsbDevice to format.
     * @return A formatted string containing device information.
     */
    private static String formatDeviceInfo(final UsbDevice device, final String exposedDeviceName) {
        StringBuilder deviceInfo = new StringBuilder();
        deviceInfo.append(exposedDeviceName).append("\t")
                 .append(device.getProductName()).append("\t")
                 .append(device.getManufacturerName()).append("\t")
                 .append(device.getSerialNumber()).append("\t")
                 .append(device.getProductId()).append("\t")
                 .append(device.getVendorId());

        QGCLogger.d(TAG, "Formatted Device Info: " + deviceInfo.toString());

        return deviceInfo.toString();
    }

    /**
     * Opens a USB serial device.
     *
     * @param deviceName The name of the device to open.
     * @param classPtr   A native pointer associated with the device.
     * @return The device ID if successful, or BAD_DEVICE_ID if failed.
     */
    public static int open(final String deviceName, final long classPtr) {
        final DevicePortSpec spec = parseDevicePortSpec(deviceName);
        UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        UsbDevice device = driver.getDevice();
        final int physicalDeviceId = device.getDeviceId();
        final int resourceId = getOrCreateResourceId(physicalDeviceId, spec.portIndex);
        if (!deviceResourcesMap.containsKey(resourceId)) {
            deviceResourcesMap.put(resourceId, new UsbDeviceResources(driver, spec.portIndex));
        }

        final UsbDeviceResources resources = deviceResourcesMap.get(resourceId);
        if (resources != null) {
            resources.driver = driver;
            resources.portIndex = spec.portIndex;
            resources.physicalDeviceId = physicalDeviceId;
            resources.baseDeviceName = device.getDeviceName();
            resources.classPtr = classPtr;
        }

        final UsbSerialPort port = getPortFromDriver(driver, spec.portIndex);
        if (port == null) {
            QGCLogger.w(TAG, "No port " + spec.portIndex + " available on device " + deviceName);
            return BAD_DEVICE_ID;
        }

        if (!openDriver(port, device, resourceId, classPtr)) {
            QGCLogger.e(TAG, "Failed to open driver for device " + deviceName);
            emitDeviceException(classPtr, "Failed to open driver for device: " + deviceName);
            final UsbDeviceResources failedResources = deviceResourcesMap.get(resourceId);
            if (failedResources != null && failedResources.ioManager == null) {
                deviceResourcesMap.remove(resourceId);
                removeResourceMapping(resourceId);
            }
            return BAD_DEVICE_ID;
        }

        if (!createIoManager(resourceId, port, classPtr)) {
            try {
                port.close();
            } catch (IOException e) {
                QGCLogger.e(TAG, "Error closing port after IO manager failure", e);
            }
            deviceResourcesMap.remove(resourceId);
            removeResourceMapping(resourceId);
            return BAD_DEVICE_ID;
        }

        QGCLogger.d(TAG, "Port open successful: " + port.toString());
        return resourceId;
    }

    /**
     * Opens the driver for a specific USB serial port.
     *
     * @param port     The UsbSerialPort to open.
     * @param device   The UsbDevice associated with the port.
     * @param deviceId The device ID.
     * @param classPtr A native pointer associated with the device.
     * @return True if successful, false otherwise.
     */
    private static boolean openDriver(final UsbSerialPort port, final UsbDevice device, final int deviceId, final long classPtr) {
        if (port == null) {
            QGCLogger.w(TAG, "Null UsbSerialPort for device " + device.getDeviceName());
            emitDeviceException(classPtr, "No serial port available for device: " + device.getDeviceName());
            return false;
        }

        if (port.isOpen()) {
            QGCLogger.d(TAG, "Port already open for device ID " + deviceId);
            return true;
        }

        UsbDeviceConnection connection = usbManager.openDevice(device);
        if (connection == null) {
            QGCLogger.w(TAG, "No Usb Device Connection");
            emitDeviceException(classPtr, "No USB device connection for device: " + device.getDeviceName());
            return false;
        }

        try {
            port.open(connection);
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error opening driver for device " + device.getDeviceName(), ex);
            emitDeviceException(classPtr, "Error opening driver: " + ex.getMessage());
            connection.close();
            return false;
        }

        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources != null) {
            resources.fileDescriptor = connection.getFileDescriptor();
        }

        QGCLogger.d(TAG, "Port Driver open successful");
        return true;
    }

    /**
     * Creates and initializes the SerialInputOutputManager for a device.
     *
     * @param deviceId The device ID.
     * @param port     The UsbSerialPort to manage.
     * @param classPtr A native pointer associated with the device.
     * @return True if successful, false otherwise.
     */
    private static boolean createIoManager(final int deviceId, final UsbSerialPort port, final long classPtr) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return false;
        }

        if (resources.ioManager != null) {
            QGCLogger.i(TAG, "IO Manager already exists for device ID " + deviceId);
            return true;
        }

        if (port == null) {
            QGCLogger.w(TAG, "Cannot create USB serial IO manager with null port for device ID " + deviceId);
            return false;
        }

        QGCSerialListener listener = new QGCSerialListener(classPtr);
        SerialInputOutputManager ioManager = new SerialInputOutputManager(port, listener);

        int readBufferSize = READ_BUF_SIZE;
        final UsbEndpoint readEndpoint = port.getReadEndpoint();
        if (readEndpoint != null) {
            readBufferSize = Math.max(readEndpoint.getMaxPacketSize(), READ_BUF_SIZE);
        }
        ioManager.setReadBufferSize(readBufferSize);

        QGCLogger.d(TAG, "Read Buffer Size: " + ioManager.getReadBufferSize());
        QGCLogger.d(TAG, "Write Buffer Size: " + ioManager.getWriteBufferSize());

        try {
            ioManager.setReadTimeout(0);
            ioManager.setReadQueue(2);
            ioManager.setWriteTimeout(0);
            ioManager.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager configuration error:", e);
            return false;
        }

        resources.ioManager = ioManager;
        QGCLogger.d(TAG, "Serial I/O Manager created for device ID " + deviceId);
        return true;
    }

    /**
     * Starts the SerialInputOutputManager for a specific device.
     *
     * @param deviceId The device ID.
     * @return True if successful or already running, false otherwise.
     */
    public static boolean startIoManager(final int deviceId) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return false;
        }

        if (resources.ioManager == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return false;
        }

        SerialInputOutputManager.State ioState = resources.ioManager.getState();
        if (ioState == SerialInputOutputManager.State.RUNNING) {
            return true;
        }

        try {
            resources.ioManager.start();
            QGCLogger.d(TAG, "Serial I/O Manager started for device ID " + deviceId);
            return true;
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager Start exception:", e);
            return false;
        }
    }

    /**
     * Stops the SerialInputOutputManager for a specific device.
     *
     * @param deviceId The device ID.
     * @return True if successful or already stopped, false otherwise.
     */
    public static boolean stopIoManager(final int deviceId) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            return false;
        }

        if (resources.ioManager == null) {
            return false;
        }

        SerialInputOutputManager.State ioState = resources.ioManager.getState();
        if (ioState == SerialInputOutputManager.State.STOPPED || ioState == SerialInputOutputManager.State.STOPPING) {
            return true;
        }

        resources.ioManager.stop();
        QGCLogger.d(TAG, "Serial I/O Manager stopped for device ID " + deviceId);
        return true;
    }

    /**
     * Checks if the SerialInputOutputManager is running for a specific device.
     *
     * @param deviceId The device ID.
     * @return True if running, false otherwise.
     */
    public static boolean ioManagerRunning(final int deviceId) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            return false;
        }

        if (resources.ioManager == null) {
            return false;
        }

        SerialInputOutputManager.State ioState = resources.ioManager.getState();
        return (ioState == SerialInputOutputManager.State.RUNNING);
    }

    /**
     * Closes the USB serial device.
     *
     * @param deviceId The device ID.
     * @return True if successful, false otherwise.
     */
    public static boolean close(int deviceId) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            QGCLogger.d(TAG, "Close requested for already cleaned device ID " + deviceId);
            removeResourceMapping(deviceId);
            return true;
        }

        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to close a null port for device ID " + deviceId);
            deviceResourcesMap.remove(deviceId);
            removeResourceMapping(deviceId);
            return true;
        }

        if (!port.isOpen()) {
            QGCLogger.d(TAG, "Close requested for already closed device ID " + deviceId);
            deviceResourcesMap.remove(deviceId);
            removeResourceMapping(deviceId);
            return true;
        }

        stopIoManager(deviceId);

        try {
            port.close();
            QGCLogger.d(TAG, "Device " + deviceId + " closed successfully.");
            return true;
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error closing driver:", ex);
            return false;
        } finally {
            deviceResourcesMap.remove(deviceId);
            removeResourceMapping(deviceId);
        }
    }

    /**
     * Writes data to the USB serial device.
     *
     * @param deviceId    The device ID.
     * @param data        The byte array of data to write.
     * @param length      The number of bytes to write.
     * @param timeoutMSec The timeout in milliseconds.
     * @return The number of bytes written, or -1 if failed.
     */
    public static int write(final int deviceId, final byte[] data, final int length, final int timeoutMSec) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "write");
        if (port == null) {
            return -1;
        }

        try {
            port.write(data, length, timeoutMSec);
            return length;
        } catch (final SerialTimeoutException e) {
            QGCLogger.e(TAG, "Write timeout occurred", e);
            return -1;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error writing data", e);
            return -1;
        }
    }

    /**
     * Writes data asynchronously to the USB serial device.
     *
     * @param deviceId    The device ID.
     * @param data        The byte array of data to write.
     * @param timeoutMSec The timeout in milliseconds.
     * @return The number of bytes written, or -1 if failed.
     */
    public static int writeAsync(final int deviceId, final byte[] data, final int timeoutMSec) {
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.ioManager == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return -1;
        }

        if (resources.ioManager.getReadTimeout() == 0) {
            QGCLogger.w(TAG, "Read Timeout is 0 for writeAsync");
        }

        resources.ioManager.setWriteTimeout(timeoutMSec);
        resources.ioManager.writeAsync(data);

        return data.length;
    }

    /**
     * Reads data from the USB serial device.
     *
     * @param deviceId The device ID.
     * @param length   The number of bytes to read.
     * @param timeoutMs The timeout in milliseconds.
     * @return A byte array containing the read data.
     */
    public static byte[] read(final int deviceId, final int length, final int timeoutMs) {
        if (timeoutMs < 200) {
            QGCLogger.w(TAG, "Read with timeout less than recommended minimum of 200ms");
        }

        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "read");
        if (port == null) {
            return new byte[]{};
        }

        byte[] buffer = new byte[length];
        int bytesRead = 0;

        try {
            bytesRead = port.read(buffer, timeoutMs);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error reading data", e);
        }

        if (bytesRead < length) {
            return Arrays.copyOf(buffer, bytesRead);
        }

        return buffer;
    }

    /**
     * Sets the parameters on an open USB serial port.
     *
     * @param deviceId The device ID.
     * @param baudRate The baud rate (e.g., 9600, 115200).
     * @param dataBits The number of data bits (5, 6, 7, 8).
     * @param stopBits The number of stop bits (1, 2).
     * @param parity   The parity setting (0: None, 1: Odd, 2: Even).
     * @return True if successful, false otherwise.
     */
    public static boolean setParameters(final int deviceId, final int baudRate, final int dataBits, final int stopBits, final int parity) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set parameters");
        if (port == null) {
            return false;
        }

        try {
            port.setParameters(baudRate, dataBits, stopBits, parity);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting parameters" + ": " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting parameters", e);
            return false;
        }

        return true;
    }

    private static boolean getControlLine(int deviceId, UsbSerialPort.ControlLine controlLine) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "get " + controlLine);
        if (port == null) {
            return false;
        }

        if (!isControlLineSupported(port, controlLine)) {
            QGCLogger.w(TAG, "Getting " + controlLine + " Not Supported");
            return false;
        }

        try {
            switch (controlLine) {
                case CD:
                    return port.getCD();
                case CTS:
                    return port.getCTS();
                case DSR:
                    return port.getDSR();
                case DTR:
                    return port.getDTR();
                case RI:
                    return port.getRI();
                case RTS:
                    return port.getRTS();
                default:
                    return false;
            }
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error getting " + controlLine + ": " + e);
            return false;
        }  catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting " + controlLine, e);
            return false;
        }
    }

    private static boolean setControlLine(int deviceId, UsbSerialPort.ControlLine controlLine, boolean on) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set " + controlLine);
        if (port == null) {
            return false;
        }

        if (!isControlLineSupported(port, controlLine)) {
            QGCLogger.e(TAG, "Setting " + controlLine + " Not Supported");
            return false;
        }

        try {
            switch (controlLine) {
                case DTR:
                    port.setDTR(on);
                    break;
                case RTS:
                    port.setRTS(on);
                    break;
                default:
                    QGCLogger.w(TAG, "Setting " + controlLine + " is not supported via this method.");
                    return false;
            }
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting " + controlLine + ": " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting " + controlLine, e);
            return false;
        }

        return true;
    }

    /**
     * Checks if a specific control line is supported by the device.
     *
     * @param port        The UsbSerialPort.
     * @param controlLine The control line to check.
     * @return True if supported, false otherwise.
     */
    private static boolean isControlLineSupported(final UsbSerialPort port, final UsbSerialPort.ControlLine controlLine) {
        EnumSet<UsbSerialPort.ControlLine> supportedControlLines;

        try {
            supportedControlLines = port.getSupportedControlLines();
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting supported control lines", e);
            return false;
        }

        return supportedControlLines.contains(controlLine);
    }

    /**
     * Retrieves the carrier detect (CD) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if CD is active, false otherwise.
     */
    public static boolean getCarrierDetect(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.CD);
    }

    /**
     * Retrieves the clear to send (CTS) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if CTS is active, false otherwise.
     */
    public static boolean getClearToSend(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.CTS);
    }

    /**
     * Retrieves the data set ready (DSR) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if DSR is active, false otherwise.
     */
    public static boolean getDataSetReady(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.DSR);
    }

    /**
     * Retrieves the data terminal ready (DTR) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if DTR is active, false otherwise.
     */
    public static boolean getDataTerminalReady(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.DTR);
    }

    /**
     * Sets the data terminal ready (DTR) flag on the device.
     *
     * @param deviceId The device ID.
     * @param on       True to set DTR, false to clear.
     * @return True if successful, false otherwise.
     */
    public static boolean setDataTerminalReady(final int deviceId, final boolean on) {
        return setControlLine(deviceId, UsbSerialPort.ControlLine.DTR, on);
    }

    /**
     * Retrieves the request to send (RTS) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if RTS is active, false otherwise.
     */
    public static boolean getRequestToSend(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.RTS);
    }

    /**
     * Sets the request to send (RTS) flag on the device.
     *
     * @param deviceId The device ID.
     * @param on       True to set RTS, false to clear.
     * @return True if successful, false otherwise.
     */
    public static boolean setRequestToSend(final int deviceId, final boolean on) {
        return setControlLine(deviceId, UsbSerialPort.ControlLine.RTS, on);
    }

    /**
     * Retrieves the ring indicator (RI) flag from the device.
     *
     * @param deviceId The device ID.
     * @return True if RI is active, false otherwise.
     */
    public static boolean getRingIndicator(final int deviceId) {
        return getControlLine(deviceId, UsbSerialPort.ControlLine.RI);
    }

    /**
     * Retrieves the supported control lines from the device.
     *
     * @param deviceId The device ID.
     * @return An array of control line ordinals.
     */
    public static int[] getControlLines(final int deviceId) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "get control lines");
        if (port == null) {
            return new int[]{};
        }

        EnumSet<UsbSerialPort.ControlLine> currentControlLines;

        try {
            currentControlLines = port.getControlLines();
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error getting control lines: " + e);
            return new int[]{};
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting control lines", e);
            return new int[]{};
        }

        int[] lines = currentControlLines.stream().mapToInt(UsbSerialPort.ControlLine::ordinal).toArray();
        return lines;
    }

    /**
     * Retrieves the current flow control setting from the device.
     *
     * @param deviceId The device ID.
     * @return The flow control ordinal, or 0 if not supported.
     */
    public static int getFlowControl(final int deviceId) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "get flow control");
        if (port == null) {
            return 0;
        }

        EnumSet<UsbSerialPort.FlowControl> supportedFlowControl = port.getSupportedFlowControl();
        if (supportedFlowControl.isEmpty()) {
            QGCLogger.e(TAG, "Flow Control Not Supported");
            return 0;
        }

        UsbSerialPort.FlowControl flowControl = port.getFlowControl();
        return flowControl.ordinal();
    }

    /**
     * Sets the flow control setting on the device.
     *
     * @param deviceId    The device ID.
     * @param flowControl The flow control ordinal.
     * @return True if successful, false otherwise.
     */
    public static boolean setFlowControl(final int deviceId, final int flowControl) {
        if (getFlowControl(deviceId) == flowControl) {
            return true;
        }

        if (flowControl < 0 || flowControl >= UsbSerialPort.FlowControl.values().length) {
            QGCLogger.w(TAG, "Invalid flow control ordinal " + flowControl);
            return false;
        }

        UsbSerialPort.FlowControl flowControlEnum = UsbSerialPort.FlowControl.values()[flowControl];
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set flow control");
        if (port == null) {
            return false;
        }

        EnumSet<UsbSerialPort.FlowControl> supportedFlowControl = port.getSupportedFlowControl();
        if (!supportedFlowControl.contains(flowControlEnum)) {
            QGCLogger.e(TAG, "Setting Flow Control Not Supported");
            return false;
        }

        try {
            port.setFlowControl(flowControlEnum);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting Flow Control: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting Flow Control", e);
            return false;
        }

        return true;
    }

    /**
     * Sets the break condition on the device.
     *
     * @param deviceId The device ID.
     * @param on       True to set break, false to clear break.
     * @return True if successful, false otherwise.
     */
    public static boolean setBreak(final int deviceId, final boolean on) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set break");
        if (port == null) {
            return false;
        }

        try {
            port.setBreak(on);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting break condition: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting break condition", e);
            return false;
        }

        return true;
    }

    /**
     * Purges the hardware buffers on the device.
     *
     * @param deviceId The device ID.
     * @param input    True to purge the input buffer.
     * @param output   True to purge the output buffer.
     * @return True if successful, false otherwise.
     */
    public static boolean purgeBuffers(final int deviceId, final boolean input, final boolean output) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "purge buffers");
        if (port == null) {
            return false;
        }

        try {
            port.purgeHwBuffers(input, output);
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error purging buffers: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error purging buffers", e);
            return false;
        }

        return true;
    }

    /**
     * Inner class to handle serial data callbacks.
     */
    private static class QGCSerialListener implements SerialInputOutputManager.Listener {
        private final long classPtr;

        public QGCSerialListener(long classPtr) {
            this.classPtr = classPtr;
        }

        @Override
        public void onRunError(Exception e) {
            QGCLogger.e(TAG, "Runner stopped.", e);
            emitDeviceException(classPtr, "Runner stopped: " + e.getMessage());
        }

        @Override
        public void onNewData(final byte[] data) {
            if (!isValidData(data)) {
                QGCLogger.w(TAG, "Invalid data received: " + Arrays.toString(data));
                return;
            }

            if (data.length <= MAX_NATIVE_CALLBACK_DATA_BYTES) {
                emitDeviceNewData(classPtr, data);
                return;
            }

            QGCLogger.w(TAG, "Large USB payload (" + data.length + " bytes), chunking before JNI callback");
            int offset = 0;
            while (offset < data.length) {
                final int end = Math.min(offset + MAX_NATIVE_CALLBACK_DATA_BYTES, data.length);
                emitDeviceNewData(classPtr, Arrays.copyOfRange(data, offset, end));
                offset = end;
            }
        }

        private boolean isValidData(byte[] data) {
            return ((data != null) && (data.length > 0));
        }
    }
}
