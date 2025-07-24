package org.mavlink.qgroundcontrol;

import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.content.*;
import android.hardware.usb.*;
import android.os.Process;
import android.util.Log;

import com.hoho.android.usbserial.driver.*;
import com.hoho.android.usbserial.util.*;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.*;
import java.util.concurrent.*;

public class QGCUsbSerialManager {
    private static final String TAG = QGCUsbSerialManager.class.getSimpleName();
    private static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    private static final int BAD_DEVICE_ID = 0;
    private static final int READ_BUF_SIZE = 2048;

    private static UsbManager usbManager;
    private static PendingIntent usbPermissionIntent;
    private static UsbSerialProber usbSerialProber;

    private static List<UsbSerialDriver> drivers = new CopyOnWriteArrayList<>();
    private static ConcurrentHashMap<Integer, UsbDeviceResources> deviceResourcesMap = new ConcurrentHashMap<>();

    // Native methods
    private static native void nativeDeviceHasDisconnected(final long classPtr);
    public static native void nativeDeviceException(final long classPtr, final String message);
    public static native void nativeDeviceNewData(final long classPtr, final byte[] data);
    private static native void nativeUpdateAvailableJoysticks();

    /**
     * Encapsulates all resources associated with a USB device.
     */
    private static class UsbDeviceResources {
        UsbSerialDriver driver;
        SerialInputOutputManager ioManager;
        int fileDescriptor;
        long classPtr;

        UsbDeviceResources(UsbSerialDriver driver) {
            this.driver = driver;
        }
    }

    /**
     * Initializes the UsbSerialManager. Should be called once, typically from QGCActivity.onCreate().
     *
     * @param context The application context.
     */
    public static void initialize(Context context) {
        if (usbManager != null) {
            // Already initialized
            return;
        }

        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        setupUsbPermissionIntent(context);
        registerUsbReceiver(context);
        usbSerialProber = UsbSerialProber.getDefaultProber();
        updateCurrentDrivers();
    }

    /**
     * Cleans up resources by unregistering the BroadcastReceiver.
     * Should be called when the manager is no longer needed, typically from QGCActivity.onDestroy().
     */
    public static void cleanup(Context context) {
        try {
            context.unregisterReceiver(usbReceiver);
            QGCLogger.i(TAG, "BroadcastReceiver unregistered successfully.");
        } catch (final IllegalArgumentException e) {
            QGCLogger.w(TAG, "Receiver not registered: " + e.getMessage());
        }
    }

    /**
     * Sets up the PendingIntent for USB permission requests.
     *
     * @param context The application context.
     */
    private static void setupUsbPermissionIntent(Context context) {
        int intentFlags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            intentFlags = PendingIntent.FLAG_IMMUTABLE;
        }
        usbPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), intentFlags);
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
        // TODO: Move bluetooth handling back to QGCActivity
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);

        try {
            if (android.os.Build.VERSION.SDK_INT >=
                android.os.Build.VERSION_CODES.TIRAMISU) {
                int flags = Context.RECEIVER_NOT_EXPORTED;
                context.registerReceiver(usbReceiver, filter, flags);
            } else {
                context.registerReceiver(usbReceiver, filter);
            }

            QGCLogger.i(TAG, "BroadcastReceiver registered successfully.");
        } catch (Exception e) {
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

            try {
                nativeUpdateAvailableJoysticks();
            } catch (final Exception ex) {
                QGCLogger.e(TAG, "Exception nativeUpdateAvailableJoysticks()", ex);
            }
        }
    };

    /**
     * Handles USB permission results.
     *
     * @param intent The intent containing permission data.
     */
    private static void handleUsbPermission(final Intent intent) {
        UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null) {
            int deviceId = device.getDeviceId();
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                QGCLogger.i(TAG, "Permission granted to " + device.getDeviceName());
                addOrUpdateDevice(device);
            } else {
                QGCLogger.i(TAG, "Permission denied for " + device.getDeviceName());
                UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
                if (resources != null) {
                    nativeDeviceException(resources.classPtr, "USB Permission Denied");
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
        UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null) {
            int deviceId = device.getDeviceId();
            UsbDeviceResources resources = deviceResourcesMap.remove(deviceId);
            if (resources != null) {
                nativeDeviceHasDisconnected(resources.classPtr);
            }
            QGCLogger.i(TAG, "Device detached: " + device.getDeviceName());
        }
    }

    /**
     * Handles USB device detachment events.
     *
     * @param intent The intent containing device data.
     */
    private static void handleUsbDeviceAttached(final Intent intent) {
        UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
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
        return drivers.stream().anyMatch(driver -> driver.getDevice().getDeviceName().equals(name));
    }

    /**
     * Checks if a device name is currently open.
     *
     * @param name The device name to check.
     * @return True if open, false otherwise.
     */
    public static boolean isDeviceNameOpen(final String name) {
        int deviceId = getDeviceId(name);
        UsbSerialPort port = findPortByDeviceId(deviceId);
        return (port != null && port.isOpen());
    }

    /**
     * Retrieves the device ID for a given device name.
     *
     * @param deviceName The device name.
     * @return The device ID, or BAD_DEVICE_ID if not found.
     */
    public static int getDeviceId(final String deviceName) {
        UsbSerialDriver driver = findDriverByDeviceName(deviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to get ID of unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        UsbDevice device = driver.getDevice();
        return device.getDeviceId();
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
     *
     * @param deviceId The device ID to update, or -1 to update all.
     */
    private static void updateCurrentDrivers() {
        final List<UsbSerialDriver> currentDrivers = usbSerialProber.findAllDrivers(usbManager);
        if (currentDrivers.isEmpty()) {
            return;
        }

        removeStaleDrivers(currentDrivers);
        addNewDrivers(currentDrivers);
    }

    /**
     * Removes drivers that are no longer connected.
     *
     * @param currentDrivers The list of currently connected drivers.
     */
    private static void removeStaleDrivers(final List<UsbSerialDriver> currentDrivers) {
        for (int i = drivers.size() - 1; i >= 0; i--) {
            UsbSerialDriver existingDriver = drivers.get(i);
            boolean found = currentDrivers.stream()
                    .anyMatch(currentDriver -> currentDriver.getDevice().getDeviceId() == existingDriver.getDevice().getDeviceId());

            if (!found) {
                int deviceId = existingDriver.getDevice().getDeviceId();
                deviceResourcesMap.remove(deviceId);
                drivers.remove(i);
                QGCLogger.i(TAG, "Removed stale driver for device ID " + deviceId);
            }
        }
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

        drivers.add(newDriver);
        deviceResourcesMap.put(device.getDeviceId(), new UsbDeviceResources(newDriver));
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
        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        return (resources != null) ? resources.driver : null;
    }

    /**
     * Finds a USB serial driver by its device name.
     *
     * @param deviceName The device name.
     * @return The corresponding UsbSerialDriver or null if not found.
     */
    private static UsbSerialDriver findDriverByDeviceName(final String deviceName) {
        for (UsbDeviceResources resources : deviceResourcesMap.values()) {
            if (resources.driver.getDevice().getDeviceName().equals(deviceName)) {
                return resources.driver;
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
        final int portIndex = 0;

        if (deviceId == BAD_DEVICE_ID) {
            QGCLogger.w(TAG, "Finding port failed for invalid Device ID " + deviceId);
            return null;
        }

        UsbSerialDriver driver = findDriverByDeviceId(deviceId);
        if (driver == null) {
            QGCLogger.w(TAG, "No driver found on device ID " + deviceId);
            return null;
        }

        List<UsbSerialPort> ports = driver.getPorts();
        if (ports.isEmpty()) {
            QGCLogger.w(TAG, "No ports available on device ID " + deviceId);
            return null;
        }

        if (portIndex < 0 || portIndex >= ports.size()) {
            QGCLogger.w(TAG, "Invalid port index " + portIndex + " for device ID " + deviceId);
            return null;
        }

        return ports.get(portIndex);
    }

    /**
     * Retrieves information about all available USB serial devices.
     *
     * @return An array of device information strings or null if no devices are available.
     */
    public static String[] availableDevicesInfo() {
        // updateCurrentDrivers();

        if (usbManager.getDeviceList().size() < 1) {
            return null;
        }

        final List<String> deviceInfoList = new ArrayList<>();

        for (final UsbDevice device : usbManager.getDeviceList().values()) {
            final String deviceInfo = formatDeviceInfo(device);
            deviceInfoList.add(deviceInfo);
        }

        return deviceInfoList.toArray(new String[0]);
    }

    /**
     * Formats device information into a standardized string.
     *
     * @param device The UsbDevice to format.
     * @return A formatted string containing device information.
     */
    private static String formatDeviceInfo(final UsbDevice device) {
        StringBuilder deviceInfo = new StringBuilder();
        deviceInfo.append(device.getDeviceName()).append(":")
                 .append(device.getProductName()).append(":")
                 .append(device.getManufacturerName()).append(":")
                 .append(device.getSerialNumber()).append(":")
                 .append(device.getProductId()).append(":")
                 .append(device.getVendorId());

        QGCLogger.i(TAG, "Formatted Device Info: " + deviceInfo.toString());

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
        UsbSerialDriver driver = findDriverByDeviceName(deviceName);
        if (driver == null) {
            QGCLogger.w(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        List<UsbSerialPort> ports = driver.getPorts();
        if (ports.isEmpty()) {
            QGCLogger.w(TAG, "No ports available on device " + deviceName);
            return BAD_DEVICE_ID;
        }

        UsbDevice device = driver.getDevice();
        int deviceId = device.getDeviceId();
        UsbSerialPort port = findPortByDeviceId(deviceId);

        if (!openDriver(port, device, deviceId, classPtr)) {
            QGCLogger.e(TAG, "Failed to open driver for device " + deviceName);
            nativeDeviceException(classPtr, "Failed to open driver for device: " + deviceName);
            return BAD_DEVICE_ID;
        }

        if (!createIoManager(deviceId, port, classPtr)) {
            return BAD_DEVICE_ID;
        }

        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources != null) {
            resources.classPtr = classPtr;
        }

        QGCLogger.d(TAG, "Port open successful: " + port.toString());
        return deviceId;
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
        UsbDeviceConnection connection = usbManager.openDevice(device);
        if (connection == null) {
            QGCLogger.w(TAG, "No Usb Device Connection");
            nativeDeviceException(classPtr, "No USB device connection for device: " + device.getDeviceName());
            return false;
        }

        try {
            port.open(connection);
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error opening driver for device " + device.getDeviceName(), ex);
            nativeDeviceException(classPtr, "Error opening driver: " + ex.getMessage());
            return false;
        }

        UsbDeviceResources resources = deviceResourcesMap.get(deviceId);
        if (resources != null) {
            resources.fileDescriptor = connection.getFileDescriptor();
        }

        if (!setParameters(deviceId, 9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)) {
            return false;
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
        if (deviceResourcesMap.get(deviceId) == null) {
            QGCLogger.w(TAG, "No resources found for device ID " + deviceId);
            return false;
        }

        if (deviceResourcesMap.get(deviceId).ioManager != null) {
            QGCLogger.i(TAG, "IO Manager already exists for device ID " + deviceId);
            return true;
        }

        QGCSerialListener listener = new QGCSerialListener(classPtr);
        SerialInputOutputManager ioManager = new SerialInputOutputManager(port, listener);

        ioManager.setReadBufferSize(Math.max(port.getReadEndpoint().getMaxPacketSize(), READ_BUF_SIZE));

        QGCLogger.d(TAG, "Read Buffer Size: " + ioManager.getReadBufferSize());
        QGCLogger.d(TAG, "Write Buffer Size: " + ioManager.getWriteBufferSize());

        try {
            ioManager.setReadTimeout(0);
            ioManager.setWriteTimeout(0);
            ioManager.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final IllegalStateException e) {
            QGCLogger.e(TAG, "IO Manager configuration error:", e);
            return false;
        }

        deviceResourcesMap.get(deviceId).ioManager = ioManager;
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
        if (resources == null || resources.ioManager == null) {
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
        if (resources == null || resources.ioManager == null) {
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
        if (resources == null || resources.ioManager == null) {
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
            QGCLogger.w(TAG, "Attempted to close a non-existent device ID " + deviceId);
            return false;
        }

        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to close a null port for device ID " + deviceId);
            return false;
        }

        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to close an already closed port for device ID " + deviceId);
            return false;
        }

        stopIoManager(deviceId);
        deviceResourcesMap.remove(deviceId);

        try {
            port.close();
            QGCLogger.d(TAG, "Device " + deviceId + " closed successfully.");
            return true;
        } catch (final IOException ex) {
            QGCLogger.e(TAG, "Error closing driver:", ex);
            return false;
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to write to a null port for device ID " + deviceId);
            return -1;
        }

        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to write to a closed port for device ID " + deviceId);
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
        if (timeoutMs < 500) {
            QGCLogger.w(TAG, "Read with timeout less than recommended minimum of 200-500ms");
        }

        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to read from a null port for device ID " + deviceId);
            return new byte[]{};
        }

        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to read from a closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to set parameters to a null port for device ID " + deviceId);
            return false;
        }

        if (!port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to set parameters on a closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to get " + controlLine + " from a null port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null) {
            QGCLogger.w(TAG, "Attempted to set " + controlLine + " on a null port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null || !port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to get control lines from a null or closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null || !port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to get flow control from a null or closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);

        if (port == null || !port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to set flow control on a null or closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null || !port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to set break on a null or closed port for device ID " + deviceId);
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
        UsbSerialPort port = findPortByDeviceId(deviceId);
        if (port == null || !port.isOpen()) {
            QGCLogger.w(TAG, "Attempted to purge buffers on a null or closed port for device ID " + deviceId);
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
        private long classPtr;

        public QGCSerialListener(long classPtr) {
            this.classPtr = classPtr;
        }

        @Override
        public void onRunError(Exception e) {
            QGCLogger.e(TAG, "Runner stopped.", e);
            nativeDeviceException(classPtr, "Runner stopped: " + e.getMessage());
        }

        @Override
        public void onNewData(final byte[] data) {
            if (isValidData(data)) {
                nativeDeviceNewData(classPtr, data);
            } else {
                QGCLogger.w(TAG, "Invalid data received: " + Arrays.toString(data));
            }
        }

        private boolean isValidData(byte[] data) {
            return ((data != null) && (data.length > 0));
        }
    }
}
