package org.mavlink.qgroundcontrol;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.EnumSet;
import java.util.TimerTask;

import android.app.Activity;
import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.Process;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.WindowManager;

import com.hoho.android.usbserial.driver.*;
import com.hoho.android.usbserial.util.*;

import org.qtproject.qt.android.bindings.QtActivity;
import org.qtproject.qt.android.QtNative;

// TODO:
// UsbSerialDriver getDriver();
// UsbEndpoint getWriteEndpoint();
// UsbEndpoint getReadEndpoint();
// boolean getXON() throws IOException;
// UsbAccessory, UsbConfiguration, UsbConstants, UsbDevice, UsbDeviceConnection, UsbEndpoint, UsbInterface, UsbManager, UsbRequest

public class QGCActivity extends QtActivity
{
    private static final String TAG = QGCActivity.class.getSimpleName();
    private static final String SCREEN_BRIGHT_WAKE_LOCK_TAG = "QGroundControl"; // BuildConfig.LIBRARY_PACKAGE_NAME, Context.getPackageName()
    private static final String MULTICAST_LOCK_TAG = "QGroundControl"; // BuildConfig.LIBRARY_PACKAGE_NAME, Context.getPackageName()

    private static QGCActivity m_instance = null;
    private static Context m_context;

    private static final int BAD_DEVICE_ID = 0;
    private static final String ACTION_USB_PERMISSION = "org.mavlink.qgroundcontrol.action.USB_PERMISSION";
    private static PendingIntent m_usbPermissionIntent = null;
    private static UsbManager m_usbManager = null;
    private static List<UsbSerialDriver> m_drivers;
    private static HashMap<Integer, SerialInputOutputManager> m_serialIoManagerHashByDeviceId;
    private static HashMap<Integer, Integer> m_fileDescriptorHashByDeviceId;
    private static HashMap<Integer, Long> m_classPtrHashByDeviceId;

    private static PowerManager.WakeLock m_wakeLock;
    private static WifiManager.MulticastLock m_wifiMulticastLock;

    private final static BroadcastReceiver m_usbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent)
        {
            final String action = intent.getAction();
            Log.i(TAG, "BroadcastReceiver USB action " + action);

            if (ACTION_USB_PERMISSION.equals(action)) {
                _handleUsbPermission(intent);
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                _handleUsbDeviceDetached(intent);
            } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                _handleUsbDeviceAttached(intent);
            }

            try {
               nativeUpdateAvailableJoysticks();
            } catch (final Exception ex) {
               Log.e(TAG, "Exception nativeUpdateAvailableJoysticks()", ex);
            }
        }
    };

    private static void _handleUsbPermission(final Intent intent)
    {
        synchronized (m_instance) {
            final UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            if (device != null) {
                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    Log.i(TAG, "Permission granted to " + device.getDeviceName());
                } else {
                    Log.i(TAG, "Permission denied for " + device.getDeviceName());
                }
            }
        }
    }

    private static void _handleUsbDeviceDetached(final Intent intent)
    {
        final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null) {
            if (m_classPtrHashByDeviceId.containsKey(device.getDeviceId())) {
                nativeDeviceHasDisconnected(m_classPtrHashByDeviceId.get(device.getDeviceId()));
            }
        }
    }

    private static void _handleUsbDeviceAttached(final Intent intent)
    {
        final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null) {
            final UsbSerialDriver driver = _findDriverByDeviceId(device.getDeviceId());
            if ((driver != null) && !m_usbManager.hasPermission(device)) {
                Log.i(TAG, "Requesting permission for device " + device.getDeviceName());
                m_usbManager.requestPermission(device, m_usbPermissionIntent);
            }
        }
    }

    // Native C++ functions which connect back to QSerialPort code
    private static native void nativeDeviceHasDisconnected(final long classPtr);
    public static native void nativeDeviceException(final long classPtr, final String message);
    public static native void nativeDeviceNewData(final long classPtr, final byte[] data);
    private static native void nativeUpdateAvailableJoysticks();

    // Native C++ functions called to log output
    public static native void qgcLogDebug(final String message);
    public static native void qgcLogWarning(final String message);

    public native void nativeInit();

    // QGCActivity singleton
    public QGCActivity()
    {
        m_instance = this;
        m_drivers = new ArrayList<>();
        m_classPtrHashByDeviceId = new HashMap<>();
        m_serialIoManagerHashByDeviceId = new HashMap<>();
        m_fileDescriptorHashByDeviceId = new HashMap<>();
    }

    public void onInit(int status)
    {

    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        nativeInit();
        _acquireWakeLock();
        _keepScreenOn();
        _setupMulticastLock();
        _registerReceivers();
        _setupUsbPermissionIntent();

        m_usbManager = (UsbManager) m_instance.getSystemService(Context.USB_SERVICE);
    }

    @Override
    protected void onDestroy()
    {
        try {
            _releaseMulticastLock();
            _releaseWakeLock();
        } catch(final Exception e) {
           Log.e(TAG, "Exception onDestroy()", e);
        }

        super.onDestroy();
    }

    private void _keepScreenOn()
    {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    private void _acquireWakeLock()
    {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        m_wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, SCREEN_BRIGHT_WAKE_LOCK_TAG);
        if (m_wakeLock != null) {
            m_wakeLock.acquire();
        } else {
            Log.w(TAG, "SCREEN_BRIGHT_WAKE_LOCK not acquired!");
        }
    }

    private void _releaseWakeLock()
    {
        if (m_wakeLock != null) {
            m_wakeLock.release();
        }
    }

    // Workaround for QTBUG-73138
    private void _setupMulticastLock()
    {
        if (m_wifiMulticastLock == null) {
            final WifiManager wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
            m_wifiMulticastLock = wifi.createMulticastLock(MULTICAST_LOCK_TAG);
            m_wifiMulticastLock.setReferenceCounted(true);
        }

        m_wifiMulticastLock.acquire();
        Log.d(TAG, "Multicast lock: " + m_wifiMulticastLock.toString());
    }

    private void _releaseMulticastLock()
    {
        if (m_wifiMulticastLock != null) {
            m_wifiMulticastLock.release();
            Log.d(TAG, "Multicast lock released.");
        }
    }

    private void _registerReceivers()
    {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);
        filter.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(m_usbReceiver, filter, RECEIVER_EXPORTED);
        } else {
            registerReceiver(m_usbReceiver, filter);
        }
    }

    private void _setupUsbPermissionIntent()
    {
        int intentFlags = 0;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            intentFlags = PendingIntent.FLAG_IMMUTABLE;
        }
        m_usbPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), intentFlags);
    }

    private static UsbSerialDriver _findDriverByDeviceId(int deviceId)
    {
        for (final UsbSerialDriver driver: m_drivers) {
            if (driver.getDevice().getDeviceId() == deviceId) {
                return driver;
            }
        }

        return null;
    }

    private static UsbSerialDriver _findDriverByDeviceName(final String deviceName)
    {
        for (final UsbSerialDriver driver: m_drivers) {
            if (driver.getDevice().getDeviceName().equals(deviceName)) {
                return driver;
            }
        }

        return null;
    }

    private static UsbSerialPort _findPortByDeviceId(int deviceId)
    {
        final UsbSerialDriver driver = _findDriverByDeviceId(deviceId);
        if (driver == null) {
            return null;
        }

        final List<UsbSerialPort> ports = driver.getPorts();
        if (ports.isEmpty()) {
            Log.w(TAG, "No ports available on device ID " + deviceId);
            return null;
        }

        final UsbSerialPort port = ports.get(0);
        return port;
    }

    /*private static void _refreshUsbDevices()
    {
        final UsbSerialProber usbDefaultProber = UsbSerialProber.getDefaultProber();
        final UsbSerialProber usbQGCProber = QGCProber.getQGCProber();

        m_drivers.clear();
        for (final UsbDevice device : m_usbManager.getDeviceList().values()) {
            UsbSerialDriver driver = usbDefaultProber.probeDevice(device);
            if (driver == null) {
                driver = usbQGCProber.probeDevice(device);
            }
            if (driver != null) {
                m_drivers.add(driver);
            }
        }
    }*/

    /**
     * @brief Incrementally updates the list of drivers connected to the device.
     */
    private static void _updateCurrentDrivers()
    {
        final UsbSerialProber usbDefaultProber = UsbSerialProber.getDefaultProber();
        List<UsbSerialDriver> currentDrivers = usbDefaultProber.findAllDrivers(m_usbManager);
        if (currentDrivers.isEmpty()) {
            currentDrivers = QGCProber.getQGCProber().findAllDrivers(m_usbManager);
        }

        if (currentDrivers.isEmpty()) {
            return;
        }

        _removeStaleDrivers(currentDrivers);

        _addNewDrivers(currentDrivers);
    }

    private static void _removeStaleDrivers(final List<UsbSerialDriver> currentDrivers)
    {
        for (int i = m_drivers.size() - 1; i >= 0; i--) {
            boolean found = false;
            for (final UsbSerialDriver currentDriver : currentDrivers) {
                if (m_drivers.get(i).getDevice().getDeviceId() == currentDriver.getDevice().getDeviceId()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                Log.i(TAG, "Remove stale driver " + m_drivers.get(i).getDevice().getDeviceName());
                m_drivers.remove(i);
            }
        }
    }

    private static void _addNewDrivers(final List<UsbSerialDriver> currentDrivers)
    {
        for (final UsbSerialDriver newDriver : currentDrivers) {
            boolean found = false;
            for (final UsbSerialDriver driver : m_drivers) {
                if (newDriver.getDevice().getDeviceId() == driver.getDevice().getDeviceId()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                _addDriver(newDriver);
            }
        }
    }

    private static void _addDriver(final UsbSerialDriver newDriver)
    {
        final UsbDevice device = newDriver.getDevice();
        final String deviceName = device.getDeviceName();

        m_drivers.add(newDriver);
        Log.i(TAG, "Adding new driver " + deviceName);

        if (m_usbManager.hasPermission(device)) {
            Log.i(TAG, "Already have permission to use device " + deviceName);
        } else {
            Log.i(TAG, "Requesting permission to use device " + deviceName);
            m_usbManager.requestPermission(device, m_usbPermissionIntent);
        }
    }

    /**
     * @brief Returns an array of device info for each device.
     *
     * @return String[] Device info in the format "DeviceName:ProductName:Manufacturer:SerialNumber:ProductId:VendorId".
     */
    public static String[] availableDevicesInfo()
    {
        _updateCurrentDrivers();

        if (m_usbManager.getDeviceList().size() < 1) {
            return null;
        }

        final List<String> deviceInfoList = new ArrayList<>();

        for (final UsbDevice device : m_usbManager.getDeviceList().values()) {
            final String deviceInfo = _formatDeviceInfo(device);
            deviceInfoList.add(deviceInfo);
        }

        return deviceInfoList.toArray(new String[0]);
    }

    private static String _formatDeviceInfo(final UsbDevice device)
    {
        String deviceInfo = "";
        deviceInfo += device.getDeviceName() + ":";
        deviceInfo += device.getProductName() + ":";
        deviceInfo += device.getManufacturerName() + ":";
        deviceInfo += device.getSerialNumber() + ":";
        deviceInfo += device.getProductId() + ":";
        deviceInfo += device.getVendorId() + ":";

        Log.i(TAG, "_formatDeviceInfo " + deviceInfo);

        return deviceInfo;
    }

    /**
     * @brief Opens the specified device.
     *
     * @param classPtr Data to associate with the device and pass back through to native calls.
     * @return int Device ID.
     */
    public static int open(final Context parentContext, final String deviceName, final long classPtr)
    {
        m_context = parentContext;

        final UsbSerialDriver driver = _findDriverByDeviceName(deviceName);
        if (driver == null) {
            Log.w(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final List<UsbSerialPort> ports = driver.getPorts();
        if (ports.isEmpty()) {
            Log.w(TAG, "No ports available on device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final UsbSerialPort port = ports.get(0);

        final UsbDevice device = driver.getDevice();
        final int deviceId = device.getDeviceId();

        if (!_openDriver(port, device, deviceId, classPtr)) {
            return BAD_DEVICE_ID;
        }

        return deviceId;
    }

    private static boolean _openDriver(final UsbSerialPort port, final UsbDevice device, final int deviceId, final long classPtr)
    {
        final UsbDeviceConnection connection = m_usbManager.openDevice(device);
        if (connection == null) {
            Log.w(TAG, "No Usb Device Connection");
            // TODO: add UsbManager.requestPermission(driver.getDevice(), ..) handling here?
            return false;
        }

        try {
            port.open(connection);
        } catch (final IOException ex) {
            Log.e(TAG, "Error opening driver", ex);
            return false;
        }

        if (!setParameters(deviceId, 115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)) {
            return false;
        }

        if (!_createIoManager(deviceId, port, classPtr)) {
            return false;
        }

        m_classPtrHashByDeviceId.put(deviceId, classPtr);
        m_fileDescriptorHashByDeviceId.put(deviceId, connection.getFileDescriptor());

        /*final ArrayList<byte[]> descriptors = UsbUtils.getDescriptors(connection);
        for (byte[] descriptor : descriptors) {
            Log.d(TAG, "Descriptor: " + HexDump.toHexString(descriptor));
        }*/

        Log.d(TAG, "Port open successful");
        return true;
    }

    private static boolean _createIoManager(final int deviceId, final UsbSerialPort port, final long classPtr)
    {
        if (m_serialIoManagerHashByDeviceId.get(deviceId) != null) {
            return true;
        }

        final QGCSerialListener listener = new QGCSerialListener(classPtr);
        final SerialInputOutputManager ioManager = new SerialInputOutputManager(port, listener);

        // ioManager.setReadBufferSize(4096);
        // ioManager.setWriteBufferSize(4096);

        Log.d(TAG, "Read Buffer Size: " + ioManager.getReadBufferSize());
        Log.d(TAG, "Write Buffer Size: " + ioManager.getWriteBufferSize());

        try {
            ioManager.setReadTimeout(0);
        } catch (final IllegalStateException e) {
            Log.e(TAG, "Set Read Timeout exception:", e);
            return false;
        }

        ioManager.setWriteTimeout(0);

        try {
            ioManager.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        } catch (final IllegalStateException e) {
            Log.e(TAG, "Set Thread Priority exception:", e);
        }

        m_serialIoManagerHashByDeviceId.put(deviceId, ioManager);

        return true;
    }

    public static boolean startIoManager(final int deviceId)
    {
        if (ioManagerRunning(deviceId)) {
            return true;
        }

        final SerialInputOutputManager ioManager = m_serialIoManagerHashByDeviceId.get(deviceId);
        if (ioManager == null) {
            return false;
        }

        try {
            ioManager.start();
        } catch (final IllegalStateException e) {
            Log.e(TAG, "IO Manager Start exception:", e);
            return false;
        }

        return true;
    }

    public static boolean stopIoManager(final int deviceId)
    {
        final SerialInputOutputManager ioManager = m_serialIoManagerHashByDeviceId.get(deviceId);
        if (ioManager == null) {
            return false;
        }

        final SerialInputOutputManager.State ioState = ioManager.getState();
        if (ioState == SerialInputOutputManager.State.STOPPED || ioState == SerialInputOutputManager.State.STOPPING) {
            return true;
        }

        if (ioManagerRunning(deviceId)) {
            ioManager.stop();
        }

        return true;
    }

    public static boolean ioManagerRunning(final int deviceId)
    {
        final SerialInputOutputManager ioManager = m_serialIoManagerHashByDeviceId.get(deviceId);
        if (ioManager == null) {
            return false;
        }

        final SerialInputOutputManager.State ioState = ioManager.getState();
        return (ioState == SerialInputOutputManager.State.RUNNING);
    }

    public static int ioManagerReadBufferSize(final int deviceId)
    {
        final SerialInputOutputManager ioManager = m_serialIoManagerHashByDeviceId.get(deviceId);
        if (ioManager == null) {
            return 0;
        }

        return ioManager.getReadBufferSize();
    }

    /**
     * @brief Closes the device.
     *
     * @param deviceId ID number from the open command.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean close(int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        try {
            port.close();
        } catch (final IOException ex) {
            Log.e(TAG, "Error closing driver:", ex);
            return false;
        }

        if (stopIoManager(deviceId)) {
            m_serialIoManagerHashByDeviceId.remove(deviceId);
        }

        m_classPtrHashByDeviceId.remove(deviceId);
        m_fileDescriptorHashByDeviceId.remove(deviceId);

        return true;
    }

    /**
     * @brief Writes data to the device.
     *
     * @param deviceId ID number from the open command.
     * @param data Byte array of data to write.
     * @param timeoutMsec Amount of time in milliseconds to wait for the write to occur.
     * @return int Number of bytes written.
     */
    public static int write(final int deviceId, final byte[] data, final int length, final int timeoutMSec)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return -1;
        }

        try {
            port.write(data, length, timeoutMSec);
        } catch (final SerialTimeoutException e) {
            Log.e(TAG, "Write timeout occurred", e);
            return -1;
        } catch (final IOException e) {
            Log.e(TAG, "Error writing data", e);
            return -1;
        }

        return length;
    }

    /**
     * Writes data to the device asynchronously.
     *
     * @param deviceId ID number from the open command.
     * @param data Byte array of data to write.
     */
    public static int writeAsync(final int deviceId, final byte[] data, final int timeoutMSec)
    {
        final SerialInputOutputManager ioManager = m_serialIoManagerHashByDeviceId.get(deviceId);
        if (ioManager == null) {
            Log.w(TAG, "IO Manager not found for device ID " + deviceId);
            return -1;
        }

        if (ioManager.getReadTimeout() == 0) {
            return -1;
        }

        ioManager.setWriteTimeout(timeoutMSec);

        ioManager.writeAsync(data);

        return data.length;
    }

    public static byte[] read(final int deviceId, final int length, final int timeoutMs)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return new byte[] {};
        }

        byte[] buffer = new byte[length];
        int bytesRead = 0;

        try {
            // TODO: Use bytesRead
            bytesRead = port.read(buffer, timeoutMs);
        } catch (final IOException e) {
            Log.e(TAG, "Error reading data", e);
        }

        return buffer;
    }

    public static boolean isDeviceNameValid(final String name)
    {
        for (final UsbSerialDriver driver: m_drivers) {
            if (driver.getDevice().getDeviceName().equals(name)) {
                return true;
            }
        }

        return false;
    }

    public static boolean isDeviceNameOpen(final String name)
    {
        for (final UsbSerialDriver driver: m_drivers) {
            final List<UsbSerialPort> ports = driver.getPorts();
            if (!ports.isEmpty() && name.equals(driver.getDevice().getDeviceName()) && ports.get(0).isOpen()) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Get Device ID.
     *
     * @param deviceName Device Name.
     * @return int Device ID.
     */
    public static int getDeviceId(final String deviceName)
    {
        final UsbSerialDriver driver = _findDriverByDeviceName(deviceName);
        if (driver == null) {
            Log.w(TAG, "Attempt to open unknown device " + deviceName);
            return BAD_DEVICE_ID;
        }

        final UsbDevice device = driver.getDevice();
        final int deviceId = device.getDeviceId();

        return deviceId;
    }

    /**
     * @brief Sets the parameters on an open port.
     *
     * @param deviceId ID number from the open command.
     * @param baudRate Decimal value of the baud rate. For example, 9600, 57600, 115200, etc.
     * @param dataBits Number of data bits. Valid numbers are 5, 6, 7, 8.
     * @param stopBits Number of stop bits. Valid numbers are 1, 2.
     * @param parity Parity setting: No Parity=0, Odd Parity=1, Even Parity=2.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean setParameters(final int deviceId, final int baudRate, final int dataBits, final int stopBits, final int parity)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        try {
            port.setParameters(baudRate, dataBits, stopBits, parity);
        } catch (final IOException e) {
            Log.e(TAG, "Error setting parameters", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error setting parameters", e);
            return false;
        }

        return true;
    }

    private static boolean _isControlLineSupported(final UsbSerialPort port, final UsbSerialPort.ControlLine controlLine)
    {
        EnumSet<UsbSerialPort.ControlLine> supportedControlLines;

        try {
            supportedControlLines = port.getSupportedControlLines();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting supported control lines", e);
            return false;
        }

        return supportedControlLines.contains(controlLine);
    }

    /**
     * @brief Gets the Carrier Detect (CD) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getCarrierDetect(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.CD)) {
            Log.e(TAG, "Getting CD Not Supported");
            return false;
        }

        boolean cd;

        try {
            cd = port.getCD();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting CD", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting CD", e);
            return false;
        }

        return cd;
    }

    /**
     * @brief Gets the Clear To Send (CTS) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getClearToSend(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.CTS)) {
            Log.e(TAG, "Getting CTS Not Supported");
            return false;
        }

        boolean cts;

        try {
            cts = port.getCTS();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting CTS", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting CTS", e);
            return false;
        }

        return cts;
    }

    /**
     * @brief Gets the Data Set Ready (DSR) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getDataSetReady(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.DSR)) {
            Log.e(TAG, "Getting DSR Not Supported");
            return false;
        }

        boolean dsr;

        try {
            dsr = port.getDSR();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting DSR", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting DSR", e);
            return false;
        }

        return dsr;
    }

    /**
     * @brief Gets the Data Terminal Ready (DTR) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getDataTerminalReady(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.DTR)) {
            Log.e(TAG, "Getting DTR Not Supported");
            return false;
        }

        boolean dtr;

        try {
            dtr = port.getDTR();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting DTR", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting DTR", e);
            return false;
        }

        return dtr;
    }

    /**
     * @brief Sets the Data Terminal Ready (DTR) flag on the device.
     *
     * @param deviceId ID number from the open command.
     * @param on true to turn on, false to turn off.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean setDataTerminalReady(final int deviceId, final boolean on)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.DTR)) {
            Log.e(TAG, "Setting DTR Not Supported");
            return false;
        }

        try {
            port.setDTR(on);
        } catch (final IOException e) {
            Log.e(TAG, "Error setting DTR", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error setting DTR", e);
            return false;
        }

        return true;
    }

    /**
     * @brief Gets the Ring Indicator (RI) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getRingIndicator(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.RI)) {
            Log.e(TAG, "Getting RI Not Supported");
            return false;
        }

        boolean ri;

        try {
            ri = port.getRI();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting RI", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting RI", e);
            return false;
        }

        return ri;
    }

    /**
     * @brief Gets the Request to Send (RTS) flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean getRequestToSend(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.RTS)) {
            Log.e(TAG, "Getting RTS Not Supported");
            return false;
        }

        boolean rts;

        try {
            rts = port.getRTS();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting RTS", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error getting RTS", e);
            return false;
        }

        return rts;
    }

    /**
     * @brief Sets the Request to Send (RTS) flag.
     *
     * @param deviceId ID number from the open command.
     * @param on true to turn on, false to turn off.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean setRequestToSend(final int deviceId, final boolean on)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        if (!_isControlLineSupported(port, UsbSerialPort.ControlLine.RTS)) {
            Log.e(TAG, "Setting RTS Not Supported");
            return false;
        }

        try {
            port.setRTS(on);
        } catch (final IOException e) {
            Log.e(TAG, "Error setting RTS", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error setting RTS", e);
            return false;
        }

        return true;
    }

    /**
     * @brief Gets the Control Lines flags.
     *
     * @param deviceId ID number from the open command.
     */
    public static int[] getControlLines(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return new int[] {};
        }

        EnumSet<UsbSerialPort.ControlLine> supportedControlLines;

        try {
            supportedControlLines = port.getSupportedControlLines();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting supported control lines", e);
            return new int[] {};
        }

        if (supportedControlLines.isEmpty()) {
            Log.e(TAG, "Control Lines Not Supported");
            return new int[] {};
        }

        EnumSet<UsbSerialPort.ControlLine> controlLines;

        try {
            controlLines = port.getControlLines();
        } catch (final IOException e) {
            Log.e(TAG, "Error getting RTS", e);
            return new int[] {};
        }

        return controlLines.stream().mapToInt(UsbSerialPort.ControlLine::ordinal).toArray();
    }

    /**
     * @brief Gets the Flow Control type.
     *
     * @param deviceId ID number from the open command.
     */
    public static int getFlowControl(final int deviceId)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return 0;
        }

        final EnumSet<UsbSerialPort.FlowControl> supportedFlowControl = port.getSupportedFlowControl();

        if (supportedFlowControl.isEmpty()) {
            Log.e(TAG, "Flow Control Not Supported");
            return 0;
        }

        final UsbSerialPort.FlowControl flowControl = port.getFlowControl();

        return flowControl.ordinal();
    }

    /**
     * @brief Sets the Flow Control flag.
     *
     * @param deviceId ID number from the open command.
     */
    public static boolean setFlowControl(final int deviceId, final int flowControl)
    {
        if (getFlowControl(deviceId) == flowControl) {
            return true;
        }

        if ((flowControl < 0) || (flowControl >= UsbSerialPort.FlowControl.values().length)) {
            Log.w(TAG, "Invalid flow control ordinal " + flowControl);
            return false;
        }

        final UsbSerialPort.FlowControl flowControlEnum = UsbSerialPort.FlowControl.values()[flowControl];

        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        final EnumSet<UsbSerialPort.FlowControl> supportedFlowControl = port.getSupportedFlowControl();

        if (!supportedFlowControl.contains(flowControlEnum)) {
            Log.e(TAG, "Setting Flow Control Not Supported");
            return false;
        }

        // TODO: This shouldn't be necessary but an UnsupportedOperationException is thrown for NONE
        // if ((flowControlEnum == UsbSerialPort.FlowControl.NONE) && (supportedFlowControl.size() == 1)) {
        //     return true;
        // }

        try {
            port.setFlowControl(flowControlEnum);
        } catch (final IOException e) {
            Log.e(TAG, "Error setting Flow Control", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error setting Flow Control", e);
            return false;
        }

        return true;
    }

    /**
     * Sets the break condition on the device.
     *
     * @param deviceId ID number from the open command.
     * @param on true to set break, false to clear break.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean setBreak(final int deviceId, final boolean on)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        try {
            port.setBreak(on);
        } catch (final IOException e) {
            Log.e(TAG, "Error setting break condition", e);
            return false;
        }

        return true;
    }

    /**
     * @brief Purges the hardware buffers based on the input and output flags.
     *
     * @param deviceId ID number from the open command.
     * @param input true to purge the input buffer.
     * @param output true to purge the output buffer.
     * @return true if the operation is successful, false otherwise.
     */
    public static boolean purgeBuffers(final int deviceId, final boolean input, final boolean output)
    {
        final UsbSerialPort port = _findPortByDeviceId(deviceId);
        if (port == null) {
            return false;
        }

        try {
            port.purgeHwBuffers(input, output);
        } catch (final IOException e) {
            Log.e(TAG, "Error purging buffers", e);
            return false;
        } catch (final UnsupportedOperationException e) {
            Log.e(TAG, "Error purging buffers", e);
            return false;
        }

        return true;
    }

    /**
     * @brief Gets the native device handle (file descriptor).
     *
     * @param deviceId ID number from the open command.
     * @return int Device handle.
     */
    public static int getDeviceHandle(final int deviceId)
    {
        return m_fileDescriptorHashByDeviceId.getOrDefault(deviceId, -1);
    }

    public static String getSDCardPath()
    {
        final StorageManager storageManager = (StorageManager) m_instance.getSystemService(Activity.STORAGE_SERVICE);
        final List<StorageVolume> volumes = storageManager.getStorageVolumes();
        for (final StorageVolume vol : volumes) {
            final String path = getStorageVolumePath(vol);
            if (path != null) {
                return path;
            }
        }

        return "";
    }

    private static String getStorageVolumePath(final StorageVolume vol)
    {
        try {
            final Method methodGetPath = vol.getClass().getMethod("getPath");
            final String path = (String) methodGetPath.invoke(vol);
            if (vol.isRemovable()) {
                Log.i(TAG, "removable sd card mounted " + path);
                return path;
            } else {
                Log.i(TAG, "storage mounted " + path);
            }
        } catch (final Exception e) {
            Log.e(TAG, "Error getting storage volume path", e);
        }

        return null;
    }
}
