package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.hardware.usb.*;
import android.os.Process;
import com.hoho.android.usbserial.driver.*;
import com.hoho.android.usbserial.util.*;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class QGCUsbSerialManager implements QGCUsbPermissionHandler.Listener {

    private static final String TAG = QGCUsbSerialManager.class.getSimpleName();
    // Sentinel values: BAD_DEVICE_ID (0) for invalid device IDs,
    // getDeviceHandle() returns -1 for missing file descriptors.
    private static final int BAD_DEVICE_ID = 0;
    private static final int READ_BUF_SIZE = 2048;
    private static final int MAX_NATIVE_CALLBACK_DATA_BYTES = 16 * 1024;
    private static final String PORT_SUFFIX = "#p";

    // -------------------------------------------------------------------------
    // Singleton state
    // -------------------------------------------------------------------------

    private static final Object lifecycleLock = new Object();
    private static volatile QGCUsbSerialManager sInstance;

    /**
     * Creates the singleton.  No-op if already created.
     * Should be called once from {@code QGCActivity.onCreate()}.
     */
    public static synchronized void createInstance(final Context context) {
        synchronized (lifecycleLock) {
            if (sInstance != null) {
                return;
            }
            sInstance = new QGCUsbSerialManager(context);
        }
    }

    /** Returns the singleton, or {@code null} if not yet created. */
    public static QGCUsbSerialManager getInstance() {
        return sInstance;
    }

    /**
     * Tears down the singleton.
     * Should be called from {@code QGCActivity.onDestroy()}.
     */
    public static synchronized void destroyInstance() {
        synchronized (lifecycleLock) {
            if (sInstance == null) {
                return;
            }
            sInstance._destroy();
            sInstance = null;
        }
    }

    // -------------------------------------------------------------------------
    // Instance state (was previously all static)
    // -------------------------------------------------------------------------

    private final UsbManager usbManager;
    private final Context appContext;
    private final QGCUsbPermissionHandler permissionHandler;
    private UsbSerialProber usbSerialProber;
    private volatile NativeCallbacks nativeCallbacks = new JniNativeCallbacks();

    private final List<UsbSerialDriver> drivers = new CopyOnWriteArrayList<>();
    private final ConcurrentHashMap<Integer, UsbDeviceResources> deviceResourcesMap = new ConcurrentHashMap<>();
    private final AtomicInteger nextResourceId = new AtomicInteger(1);

    // -------------------------------------------------------------------------
    // Constructor / destructor
    // -------------------------------------------------------------------------

    // Lightweight constructor for unit tests — no Android Context, no USB, no FTDI.
    private QGCUsbSerialManager() {
    }

    private QGCUsbSerialManager(final Context context) {
        appContext = context.getApplicationContext();
        usbManager = (UsbManager) appContext.getSystemService(Context.USB_SERVICE);
        if (usbManager == null) {
            QGCLogger.e(TAG, "Failed to get UsbManager");
            permissionHandler = new QGCUsbPermissionHandler(this);
            return;
        }

        QGCFtdiSerialDriver.initialize(appContext);
        usbSerialProber = QGCUsbSerialProber.getQGCUsbSerialProber();

        permissionHandler = new QGCUsbPermissionHandler(this);
        permissionHandler.register(appContext);

        updateCurrentDrivers();
    }

    private void _destroy() {
        for (final Integer deviceId : new ArrayList<>(deviceResourcesMap.keySet())) {
            _close(deviceId);
        }

        permissionHandler.unregister(appContext);
        usbSerialProber = null;
        QGCFtdiSerialDriver.cleanup();
        drivers.clear();
        deviceResourcesMap.clear();
        nextResourceId.set(1);
    }

    // -------------------------------------------------------------------------
    // Public JNI-facing static forwarders (kept static for ABI compatibility)
    // -------------------------------------------------------------------------

    /** @deprecated use {@link #createInstance(Context)} */
    public static void initialize(final Context context) {
        createInstance(context);
    }

    /** @deprecated use {@link #destroyInstance()} */
    public static void cleanup(final Context context) {
        destroyInstance();
    }

    public static boolean isDeviceNameValid(final String name) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._isDeviceNameValid(name);
    }

    public static boolean isDeviceNameOpen(final String name) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._isDeviceNameOpen(name);
    }

    public static int getDeviceId(final String deviceName) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return BAD_DEVICE_ID;
        }
        return mgr._getDeviceId(deviceName);
    }

    public static int getDeviceHandle(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return -1;
        }
        return mgr._getDeviceHandle(deviceId);
    }

    public static String[] availableDevicesInfo() {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return null;
        }
        return mgr._availableDevicesInfo();
    }

    public static int open(final String deviceName, final long classPtr) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return BAD_DEVICE_ID;
        }
        return mgr._open(deviceName, classPtr);
    }

    public static boolean startIoManager(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._startIoManager(deviceId);
    }

    public static boolean stopIoManager(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._stopIoManager(deviceId);
    }

    public static boolean ioManagerRunning(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._ioManagerRunning(deviceId);
    }

    public static boolean close(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._close(deviceId);
    }

    public static int write(final int deviceId, final byte[] data, final int length, final int timeoutMSec) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return -1;
        }
        return mgr._write(deviceId, data, length, timeoutMSec);
    }

    public static int writeAsync(final int deviceId, final byte[] data, final int timeoutMSec) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return -1;
        }
        return mgr._writeAsync(deviceId, data, timeoutMSec);
    }

    public static byte[] read(final int deviceId, final int length, final int timeoutMs) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return new byte[]{};
        }
        return mgr._read(deviceId, length, timeoutMs);
    }

    public static boolean setParameters(final int deviceId, final int baudRate, final int dataBits,
            final int stopBits, final int parity) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return false;
        }
        return mgr._setParameters(deviceId, baudRate, dataBits, stopBits, parity);
    }

    public static boolean getCarrierDetect(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.CD);
    }

    public static boolean getClearToSend(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.CTS);
    }

    public static boolean getDataSetReady(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.DSR);
    }

    public static boolean getDataTerminalReady(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.DTR);
    }

    public static boolean setDataTerminalReady(final int deviceId, final boolean on) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._setControlLine(deviceId, UsbSerialPort.ControlLine.DTR, on);
    }

    public static boolean getRequestToSend(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.RTS);
    }

    public static boolean setRequestToSend(final int deviceId, final boolean on) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._setControlLine(deviceId, UsbSerialPort.ControlLine.RTS, on);
    }

    public static boolean getRingIndicator(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._getControlLine(deviceId, UsbSerialPort.ControlLine.RI);
    }

    public static int[] getControlLines(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return new int[]{}; }
        return mgr._getControlLines(deviceId);
    }

    public static int getFlowControl(final int deviceId) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return 0; }
        return mgr._getFlowControl(deviceId);
    }

    public static boolean setFlowControl(final int deviceId, final int flowControl) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._setFlowControl(deviceId, flowControl);
    }

    public static boolean setBreak(final int deviceId, final boolean on) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._setBreak(deviceId, on);
    }

    public static boolean purgeBuffers(final int deviceId, final boolean input, final boolean output) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) { return false; }
        return mgr._purgeBuffers(deviceId, input, output);
    }

    // -------------------------------------------------------------------------
    // QGCUsbPermissionHandler.Listener implementation
    // -------------------------------------------------------------------------

    @Override
    public void onUsbDeviceAttached(final UsbDevice device) {
        addOrUpdateDevice(device);
        updateCurrentDrivers();
    }

    @Override
    public void onUsbDeviceDetached(final UsbDevice device) {
        final int physicalDeviceId = device.getDeviceId();
        final List<Integer> resourceIds = findResourceIdsForPhysicalDevice(physicalDeviceId);
        for (final Integer resourceId : resourceIds) {
            final UsbDeviceResources resources = deviceResourcesMap.get(resourceId);
            if (resources == null) {
                continue;
            }
            final long classPtr = resources.classPtr;
            _close(resourceId);
            emitDeviceHasDisconnected(classPtr);
        }
        QGCLogger.i(TAG, "Device detached: " + device.getDeviceName());
        updateCurrentDrivers();
    }

    @Override
    public void onUsbPermissionGranted(final UsbDevice device) {
        addOrUpdateDevice(device);
        updateCurrentDrivers();
    }

    @Override
    public void onUsbPermissionDenied(final UsbDevice device) {
        final int physicalDeviceId = device.getDeviceId();
        for (final Integer resourceId : findResourceIdsForPhysicalDevice(physicalDeviceId)) {
            final UsbDeviceResources resources = deviceResourcesMap.get(resourceId);
            if (resources != null) {
                emitDeviceException(resources.classPtr, "USB Permission Denied");
            }
        }
    }

    // -------------------------------------------------------------------------
    // NativeCallbacks
    // -------------------------------------------------------------------------

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

    // Native methods
    private static native void nativeDeviceHasDisconnected(final long classPtr);
    private static native void nativeDeviceException(final long classPtr, final String message);
    private static native void nativeDeviceNewData(final long classPtr, final byte[] data);

    private void emitDeviceHasDisconnected(final long classPtr) {
        if (classPtr != 0) {
            nativeCallbacks.onDeviceHasDisconnected(classPtr);
        }
    }

    private void emitDeviceException(final long classPtr, final String message) {
        if (classPtr != 0) {
            nativeCallbacks.onDeviceException(classPtr, message);
        }
    }

    private void emitDeviceNewData(final long classPtr, final byte[] data) {
        if (classPtr != 0 && data != null && data.length > 0) {
            nativeCallbacks.onDeviceNewData(classPtr, data);
        }
    }

    // -------------------------------------------------------------------------
    // Device / driver management
    // -------------------------------------------------------------------------

    private void updateCurrentDrivers() {
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

    private void removeStaleDrivers(final List<UsbSerialDriver> currentDrivers) {
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

        for (final int physicalDeviceId : stalePhysicalIds) {
            removeAllResourcesForPhysicalDevice(physicalDeviceId);
        }
    }

    private void addNewDrivers(final List<UsbSerialDriver> currentDrivers) {
        for (final UsbSerialDriver newDriver : currentDrivers) {
            final boolean found = drivers.stream()
                    .anyMatch(d -> d.getDevice().getDeviceId() == newDriver.getDevice().getDeviceId());
            if (!found) {
                addDriver(newDriver);
            }
        }
    }

    private void addDriver(final UsbSerialDriver newDriver) {
        final UsbDevice device = newDriver.getDevice();
        final String deviceName = device.getDeviceName();

        final boolean alreadyTracked = drivers.stream()
                .anyMatch(d -> d.getDevice().getDeviceId() == device.getDeviceId());
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
            permissionHandler.requestPermission(usbManager, device);
        }
    }

    private void addOrUpdateDevice(final UsbDevice device) {
        final UsbSerialDriver driver = findDriverByDeviceId(device.getDeviceId());
        if (driver == null) {
            return;
        }
        if (usbManager.hasPermission(device)) {
            QGCLogger.i(TAG, "Already have permission to use device " + device.getDeviceName());
            addDriver(driver);
        } else {
            QGCLogger.i(TAG, "Requesting permission to use device " + device.getDeviceName());
            permissionHandler.requestPermission(usbManager, device);
        }
    }

    private UsbSerialDriver findDriverByDeviceId(final int deviceId) {
        for (final UsbSerialDriver driver : drivers) {
            if (driver.getDevice().getDeviceId() == deviceId) {
                return driver;
            }
        }
        return null;
    }

    private UsbSerialDriver findDriverByDeviceName(final String deviceName) {
        for (final UsbSerialDriver driver : drivers) {
            if (driver.getDevice().getDeviceName().equals(deviceName)) {
                return driver;
            }
        }
        return null;
    }

    // -------------------------------------------------------------------------
    // Resource-ID helpers (collapsed single-map model)
    // -------------------------------------------------------------------------

    /**
     * Returns the resource ID for the given (physicalDeviceId, portIndex) pair,
     * creating a new one if none exists.
     */
    private int getOrCreateResourceId(final int physicalDeviceId, final int portIndex) {
        final int existing = findResourceId(physicalDeviceId, portIndex);
        if (existing != BAD_DEVICE_ID) {
            return existing;
        }
        // Allocate a new ID and insert a placeholder entry so concurrent callers
        // see the reservation before the full UsbDeviceResources is populated.
        final int candidateId =
                nextResourceId.getAndUpdate(v -> (v >= Integer.MAX_VALUE) ? 1 : v + 1);

        // Double-check: another thread may have won the race while we computed candidateId.
        // putIfAbsent is not directly available since the map key is the resource ID, not
        // the port address.  A simple re-check under the map's own lock is sufficient given
        // the very small map size.
        final int recheck = findResourceId(physicalDeviceId, portIndex);
        if (recheck != BAD_DEVICE_ID) {
            return recheck;
        }

        // Insert a placeholder so subsequent lookups by (physicalDeviceId, portIndex) hit.
        // The caller will populate driver/ioManager/etc. later.
        final UsbDeviceResources placeholder = new UsbDeviceResources(null, portIndex);
        placeholder.physicalDeviceId = physicalDeviceId;
        deviceResourcesMap.put(candidateId, placeholder);

        return candidateId;
    }

    /**
     * Linear scan over deviceResourcesMap to find the resource ID for a given
     * (physicalDeviceId, portIndex) pair.  O(n) — fine for n ≤ ~5.
     */
    private int findResourceId(final int physicalDeviceId, final int portIndex) {
        for (final Map.Entry<Integer, UsbDeviceResources> entry : deviceResourcesMap.entrySet()) {
            final UsbDeviceResources res = entry.getValue();
            if (res.physicalDeviceId == physicalDeviceId && res.portIndex == portIndex) {
                return entry.getKey();
            }
        }
        return BAD_DEVICE_ID;
    }

    private List<Integer> findResourceIdsForPhysicalDevice(final int physicalDeviceId) {
        final List<Integer> ids = new ArrayList<>();
        for (final Map.Entry<Integer, UsbDeviceResources> entry : deviceResourcesMap.entrySet()) {
            if (entry.getValue().physicalDeviceId == physicalDeviceId) {
                ids.add(entry.getKey());
            }
        }
        return ids;
    }

    private void removeAllResourcesForPhysicalDevice(final int physicalDeviceId) {
        for (final Integer resourceId : findResourceIdsForPhysicalDevice(physicalDeviceId)) {
            _close(resourceId);
        }
    }

    // -------------------------------------------------------------------------
    // Port / device helpers
    // -------------------------------------------------------------------------

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

    // Package-private for test access
    static String getBaseDeviceNameForTesting(final String deviceName) {
        return parseDevicePortSpec(deviceName).baseDeviceName;
    }

    static int getPortIndexForTesting(final String deviceName) {
        return parseDevicePortSpec(deviceName).portIndex;
    }

    // Package-private test helpers — the test creates a lightweight instance directly.
    private static QGCUsbSerialManager sTestInstance;

    static void resetResourceMappingsForTesting() {
        if (sTestInstance == null) {
            sTestInstance = new QGCUsbSerialManager();
        }
        sTestInstance.deviceResourcesMap.clear();
        sTestInstance.nextResourceId.set(1);
    }

    static int getOrCreateResourceIdForTesting(final int physicalDeviceId, final int portIndex) {
        if (sTestInstance == null) {
            sTestInstance = new QGCUsbSerialManager();
        }
        return sTestInstance.getOrCreateResourceId(physicalDeviceId, portIndex);
    }

    static void removeResourceMappingForTesting(final int resourceId) {
        if (sTestInstance != null) {
            sTestInstance.deviceResourcesMap.remove(resourceId);
        }
    }

    private static String buildPortDeviceName(final UsbDevice device, final int portIndex,
            final int portCount) {
        final String baseName = device.getDeviceName();
        if (portCount <= 1 && portIndex == 0) {
            return baseName;
        }
        return baseName + PORT_SUFFIX + portIndex;
    }

    private static UsbSerialPort getPortFromDriver(final UsbSerialDriver driver,
            final int portIndex) {
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

    private UsbSerialPort findPortByDeviceId(final int deviceId) {
        if (deviceId == BAD_DEVICE_ID) {
            QGCLogger.w(TAG, "Finding port failed for invalid Device ID " + deviceId);
            return null;
        }

        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.driver == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return null;
        }

        final UsbSerialPort port = getPortFromDriver(resources.driver, resources.portIndex);
        if (port == null) {
            QGCLogger.w(TAG, "No port available on device ID " + deviceId
                    + " at port index " + resources.portIndex);
            return null;
        }

        return port;
    }

    private UsbSerialPort getPortOrWarn(final int deviceId, final String operation) {
        final UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to " + operation
                    + " on a null port for device ID " + deviceId);
        }
        return port;
    }

    private UsbSerialPort getOpenPortOrWarn(final int deviceId, final String operation) {
        final UsbSerialPort port = getPortOrWarn(deviceId, operation);
        if (port == null) {
            return null;
        }
        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to " + operation
                    + " on a closed port for device ID " + deviceId);
            return null;
        }
        return port;
    }

    // -------------------------------------------------------------------------
    // Instance method implementations (_-prefixed)
    // -------------------------------------------------------------------------

    private boolean _isDeviceNameValid(final String name) {
        final DevicePortSpec spec = parseDevicePortSpec(name);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        return getPortFromDriver(driver, spec.portIndex) != null;
    }

    private boolean _isDeviceNameOpen(final String name) {
        final DevicePortSpec spec = parseDevicePortSpec(name);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            return false;
        }
        final UsbSerialPort port = getPortFromDriver(driver, spec.portIndex);
        return (port != null && port.isOpen());
    }

    private int _getDeviceId(final String deviceName) {
        final DevicePortSpec spec = parseDevicePortSpec(deviceName);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to get ID of unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        if (getPortFromDriver(driver, spec.portIndex) == null) {
            QGCLogger.w(TAG, "Attempt to get ID of unknown port index " + spec.portIndex
                    + " for " + spec.baseDeviceName);
            return BAD_DEVICE_ID;
        }

        return getOrCreateResourceId(driver.getDevice().getDeviceId(), spec.portIndex);
    }

    private int _getDeviceHandle(final int deviceId) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        return (resources != null) ? resources.fileDescriptor : -1;
    }

    private String[] _availableDevicesInfo() {
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
                    final String exposedDeviceName =
                            buildPortDeviceName(driver.getDevice(), portIndex, portCount);
                    deviceInfoList.add(formatDeviceInfo(driver.getDevice(), exposedDeviceName));
                }
            } catch (final SecurityException e) {
                // Some integrated controllers (e.g. Siyi UNIRC7) expose a USB device for video
                // output.  Accessing device info without permission throws SecurityException;
                // swallow it to avoid log spam.
            }
        }

        return deviceInfoList.isEmpty() ? null : deviceInfoList.toArray(new String[0]);
    }

    private static String formatDeviceInfo(final UsbDevice device, final String exposedDeviceName) {
        final String info = exposedDeviceName + "\t"
                + device.getProductName() + "\t"
                + device.getManufacturerName() + "\t"
                + device.getSerialNumber() + "\t"
                + device.getProductId() + "\t"
                + device.getVendorId();
        QGCLogger.d(TAG, "Formatted Device Info: " + info);
        return info;
    }

    private int _open(final String deviceName, final long classPtr) {
        final DevicePortSpec spec = parseDevicePortSpec(deviceName);
        final UsbSerialDriver driver = findDriverByDeviceName(spec.baseDeviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final UsbDevice device = driver.getDevice();
        final UsbSerialPort port = getPortFromDriver(driver, spec.portIndex);
        if (port == null) {
            QGCLogger.w(TAG, "No port " + spec.portIndex + " available on device " + deviceName);
            return BAD_DEVICE_ID;
        }

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

        if (!openDriver(port, device, resourceId, classPtr)) {
            QGCLogger.e(TAG, "Failed to open driver for device " + deviceName);
            emitDeviceException(classPtr, "Failed to open driver for device: " + deviceName);
            final UsbDeviceResources failedResources = deviceResourcesMap.get(resourceId);
            if (failedResources != null && failedResources.ioManager == null) {
                deviceResourcesMap.remove(resourceId);
            }
            return BAD_DEVICE_ID;
        }

        if (!createIoManager(resourceId, port, classPtr)) {
            try {
                port.close();
            } catch (final IOException e) {
                QGCLogger.e(TAG, "Error closing port after IO manager failure", e);
            }
            deviceResourcesMap.remove(resourceId);
            return BAD_DEVICE_ID;
        }

        QGCLogger.d(TAG, "Port open successful: " + port.toString());
        return resourceId;
    }

    private boolean openDriver(final UsbSerialPort port, final UsbDevice device,
            final int deviceId, final long classPtr) {
        if (port == null) {
            QGCLogger.w(TAG, "Null UsbSerialPort for device " + device.getDeviceName());
            emitDeviceException(classPtr,
                    "No serial port available for device: " + device.getDeviceName());
            return false;
        }

        if (port.isOpen()) {
            QGCLogger.d(TAG, "Port already open for device ID " + deviceId);
            return true;
        }

        final UsbDeviceConnection connection = usbManager.openDevice(device);
        if (connection == null) {
            QGCLogger.w(TAG, "No Usb Device Connection");
            emitDeviceException(classPtr,
                    "No USB device connection for device: " + device.getDeviceName());
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

        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources != null) {
            resources.fileDescriptor = connection.getFileDescriptor();
        }

        QGCLogger.d(TAG, "Port Driver open successful");
        return true;
    }

    private boolean createIoManager(final int deviceId, final UsbSerialPort port,
            final long classPtr) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return false;
        }

        if (resources.ioManager != null) {
            QGCLogger.i(TAG, "IO Manager already exists for device ID " + deviceId);
            return true;
        }

        if (port == null) {
            QGCLogger.w(TAG, "Cannot create USB serial IO manager with null port for device ID "
                    + deviceId);
            return false;
        }

        final QGCSerialListener listener = new QGCSerialListener(classPtr);
        final SerialInputOutputManager ioManager =
                new SerialInputOutputManager(port, listener);

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

    private boolean _startIoManager(final int deviceId) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.ioManager == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return false;
        }

        final SerialInputOutputManager.State ioState = resources.ioManager.getState();
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

    private boolean _stopIoManager(final int deviceId) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.ioManager == null) {
            return false;
        }

        final SerialInputOutputManager.State ioState = resources.ioManager.getState();
        if (ioState == SerialInputOutputManager.State.STOPPED
                || ioState == SerialInputOutputManager.State.STOPPING) {
            return true;
        }

        resources.ioManager.stop();

        // Wait for IO thread to actually exit to prevent stale callbacks on rapid close→open
        final long deadline = System.currentTimeMillis() + 2000;
        while (resources.ioManager.getState() != SerialInputOutputManager.State.STOPPED
                && System.currentTimeMillis() < deadline) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }

        if (resources.ioManager.getState() != SerialInputOutputManager.State.STOPPED) {
            QGCLogger.w(TAG, "IO manager did not stop within timeout for device ID " + deviceId);
        }

        QGCLogger.d(TAG, "Serial I/O Manager stopped for device ID " + deviceId);
        return true;
    }

    private boolean _ioManagerRunning(final int deviceId) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.ioManager == null) {
            return false;
        }
        return resources.ioManager.getState() == SerialInputOutputManager.State.RUNNING;
    }

    private boolean _close(final int deviceId) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null) {
            QGCLogger.d(TAG, "Close requested for already cleaned device ID " + deviceId);
            return true;
        }

        final UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to close a null port for device ID " + deviceId);
            deviceResourcesMap.remove(deviceId);
            return true;
        }

        if (!port.isOpen()) {
            QGCLogger.d(TAG, "Close requested for already closed device ID " + deviceId);
            deviceResourcesMap.remove(deviceId);
            return true;
        }

        _stopIoManager(deviceId);

        try {
            port.close();
            QGCLogger.d(TAG, "Device " + deviceId + " closed successfully.");
            return true;
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error closing driver:", ex);
            return false;
        } finally {
            deviceResourcesMap.remove(deviceId);
        }
    }

    private int _write(final int deviceId, final byte[] data, final int length,
            final int timeoutMSec) {
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

    private int _writeAsync(final int deviceId, final byte[] data, final int timeoutMSec) {
        final UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources == null || resources.ioManager == null) {
            QGCLogger.w(TAG, "IO Manager not found for device ID " + deviceId);
            return -1;
        }

        resources.ioManager.setWriteTimeout(timeoutMSec);
        resources.ioManager.writeAsync(data);
        return data.length;
    }

    private byte[] _read(final int deviceId, final int length, final int timeoutMs) {
        if (timeoutMs < 200) {
            QGCLogger.w(TAG, "Read with timeout less than recommended minimum of 200ms");
        }

        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "read");
        if (port == null) {
            return new byte[]{};
        }

        final byte[] buffer = new byte[length];
        int bytesRead = 0;
        try {
            bytesRead = port.read(buffer, timeoutMs);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error reading data", e);
        }

        return (bytesRead < length) ? Arrays.copyOf(buffer, bytesRead) : buffer;
    }

    private boolean _setParameters(final int deviceId, final int baudRate, final int dataBits,
            final int stopBits, final int parity) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set parameters");
        if (port == null) {
            return false;
        }
        try {
            port.setParameters(baudRate, dataBits, stopBits, parity);
            return true;
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting parameters: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting parameters", e);
            return false;
        }
    }

    private boolean _getControlLine(final int deviceId,
            final UsbSerialPort.ControlLine controlLine) {
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
                case CD:  return port.getCD();
                case CTS: return port.getCTS();
                case DSR: return port.getDSR();
                case DTR: return port.getDTR();
                case RI:  return port.getRI();
                case RTS: return port.getRTS();
                default:  return false;
            }
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error getting " + controlLine + ": " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting " + controlLine, e);
            return false;
        }
    }

    private boolean _setControlLine(final int deviceId,
            final UsbSerialPort.ControlLine controlLine, final boolean on) {
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
                case DTR: port.setDTR(on); break;
                case RTS: port.setRTS(on); break;
                default:
                    QGCLogger.w(TAG, "Setting " + controlLine + " is not supported via this method.");
                    return false;
            }
            return true;
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting " + controlLine + ": " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting " + controlLine, e);
            return false;
        }
    }

    private static boolean isControlLineSupported(final UsbSerialPort port,
            final UsbSerialPort.ControlLine controlLine) {
        try {
            return port.getSupportedControlLines().contains(controlLine);
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting supported control lines", e);
            return false;
        }
    }

    private int[] _getControlLines(final int deviceId) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "get control lines");
        if (port == null) {
            return new int[]{};
        }
        try {
            return port.getControlLines().stream()
                    .mapToInt(UsbSerialPort.ControlLine::ordinal)
                    .toArray();
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error getting control lines: " + e);
            return new int[]{};
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error getting control lines", e);
            return new int[]{};
        }
    }

    private int _getFlowControl(final int deviceId) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "get flow control");
        if (port == null) {
            return 0;
        }
        if (port.getSupportedFlowControl().isEmpty()) {
            QGCLogger.e(TAG, "Flow Control Not Supported");
            return 0;
        }
        return port.getFlowControl().ordinal();
    }

    private boolean _setFlowControl(final int deviceId, final int flowControl) {
        if (_getFlowControl(deviceId) == flowControl) {
            return true;
        }
        if (flowControl < 0 || flowControl >= UsbSerialPort.FlowControl.values().length) {
            QGCLogger.w(TAG, "Invalid flow control ordinal " + flowControl);
            return false;
        }

        final UsbSerialPort.FlowControl flowControlEnum =
                UsbSerialPort.FlowControl.values()[flowControl];
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set flow control");
        if (port == null) {
            return false;
        }
        if (!port.getSupportedFlowControl().contains(flowControlEnum)) {
            QGCLogger.e(TAG, "Setting Flow Control Not Supported");
            return false;
        }
        try {
            port.setFlowControl(flowControlEnum);
            return true;
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting Flow Control: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting Flow Control", e);
            return false;
        }
    }

    private boolean _setBreak(final int deviceId, final boolean on) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "set break");
        if (port == null) {
            return false;
        }
        try {
            port.setBreak(on);
            return true;
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error setting break condition: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error setting break condition", e);
            return false;
        }
    }

    private boolean _purgeBuffers(final int deviceId, final boolean input, final boolean output) {
        final UsbSerialPort port = getOpenPortOrWarn(deviceId, "purge buffers");
        if (port == null) {
            return false;
        }
        try {
            port.purgeHwBuffers(input, output);
            return true;
        } catch (final UnsupportedOperationException e) {
            QGCLogger.w(TAG, "Error purging buffers: " + e);
            return false;
        } catch (final IOException e) {
            QGCLogger.e(TAG, "Error purging buffers", e);
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // Inner types
    // -------------------------------------------------------------------------

    /**
     * Encapsulates all resources associated with a single open USB serial port.
     */
    private static class UsbDeviceResources {
        UsbSerialDriver driver;
        SerialInputOutputManager ioManager;
        int fileDescriptor;
        volatile long classPtr;
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

    private static final class DevicePortSpec {
        final String baseDeviceName;
        final int portIndex;

        DevicePortSpec(final String baseDeviceName, final int portIndex) {
            this.baseDeviceName = baseDeviceName;
            this.portIndex = Math.max(0, portIndex);
        }
    }

    /**
     * Dispatches serial data callbacks from the IO manager thread to native code.
     */
    private class QGCSerialListener implements SerialInputOutputManager.Listener {
        private final long classPtr;

        QGCSerialListener(final long classPtr) {
            this.classPtr = classPtr;
        }

        @Override
        public void onRunError(final Exception e) {
            QGCLogger.e(TAG, "Runner stopped.", e);
            emitDeviceException(classPtr, "Runner stopped: " + e.getMessage());
        }

        @Override
        public void onNewData(final byte[] data) {
            if (data == null || data.length == 0) {
                QGCLogger.w(TAG, "Invalid data received");
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
    }
}
