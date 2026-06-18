package org.mavlink.qgroundcontrol.serial;

import org.mavlink.qgroundcontrol.QGCForegroundService;
import org.mavlink.qgroundcontrol.QGCLogger;

import android.content.Context;
import android.content.pm.ServiceInfo;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import org.qtproject.qt.android.UsedFromNativeCode;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;

/** Android USB serial facade: owns enumeration, permission, and attach/detach routing; per-port ops live on {@link QGCSerialPort}. */
public class QGCUsbSerialManager
        implements QGCSerialPort.PortLifecycleSink {

    private static final String TAG = QGCUsbSerialManager.class.getSimpleName();

    private static final Object singletonLock = new Object();
    private static volatile QGCUsbSerialManager sInstance;

    /** Stable Android device path plus runtime device id and usb-serial-for-android port index. */
    public static record PortAddress(String deviceName, int physicalDeviceId, int portIndex) {}

    /** Open-port registry keyed by {@link PortAddress}; detach/permission-denied use a linear name scan (open-port count is single-digit, so no fanout index). */
    static final class PortRegistry {
        private final Map<PortAddress, QGCSerialPort> portsByAddress = new ConcurrentHashMap<>();

        boolean register(final QGCSerialPort port) {
            return portsByAddress.putIfAbsent(port.address(), port) == null;
        }

        void unregister(final QGCSerialPort port) {
            portsByAddress.remove(port.address(), port);
        }

        QGCSerialPort get(final PortAddress address) {
            return portsByAddress.get(address);
        }

        List<QGCSerialPort> portsForDeviceName(final String deviceName) {
            final List<QGCSerialPort> matches = new ArrayList<>();
            for (final QGCSerialPort port : portsByAddress.values()) {
                if (port.address().deviceName().equals(deviceName)) {
                    matches.add(port);
                }
            }
            return matches;
        }

        List<QGCSerialPort> allPorts() {
            return new ArrayList<>(portsByAddress.values());
        }
    }

    /** Creates the singleton. No-op if already created. */
    public static void createInstance(final Context context) {
        synchronized (singletonLock) {
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

    /** Capture-and-null under the static lock, then run _destroy unlocked: per-port close() can sit on its lifecycleLock through the D2XX drain, which would otherwise pin the static lock. */
    @UsedFromNativeCode
    public static void destroyInstance() {
        final QGCUsbSerialManager doomed;
        synchronized (singletonLock) {
            doomed = sInstance;
            sInstance = null;
        }
        if (doomed != null) {
            doomed._destroy();
        }
    }

    private final UsbManager usbManager;
    private final Context appContext;
    private final UsbAttachDetachReceiver attachDetachReceiver;
    private final UsbSerialEnumerator enumerator;
    private final PortRegistry portRegistry = new PortRegistry();
    /** Serializes device teardown (detach vs reader-thread error) so their close/replace-driver sequences can't interleave for one device. */
    private final Object deviceTeardownLock = new Object();
    /** Guards {@link #configuredPortCount}; the lifecycle-sink callbacks fire from arbitrary threads. */
    private final Object serviceLock = new Object();
    private int configuredPortCount;

    private static final QGCForegroundService.Config USB_SERIAL_FGS_CONFIG =
            new QGCForegroundService.Config(
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE,
                    "qgc_usb_serial",
                    "USB serial",
                    "QGroundControl USB serial connection",
                    "USB serial connection active");

    private final UsbPermissionManager permissionManager;

    /** Single worker for blocking USB enumeration: keeps the probe off the broadcast main looper (ANR path) while preserving the enumerator's probe-unlocked/mutate-locked ordering. */
    private final ExecutorService usbWorker =
            Executors.newSingleThreadExecutor(r -> {
                final Thread t = new Thread(r, "QGCUsbEnumerator");
                t.setDaemon(true);
                return t;
            });

    private QGCUsbSerialManager(final Context context) {
        if (!SerialWireConstants.verifyExternalFlowControlOrdinals()) {
            throw new IllegalStateException(
                    "mik3y UsbSerialPort.FlowControl ordinals diverged from SerialWireConstants.FC_* wire values");
        }
        appContext = context.getApplicationContext();
        usbManager = (UsbManager) appContext.getSystemService(Context.USB_SERVICE);
        attachDetachReceiver = new UsbAttachDetachReceiver(this::onUsbDeviceAttached, this::onUsbDeviceDetached);
        permissionManager = new UsbPermissionManager(this::onUsbPermissionGranted, this::onUsbPermissionDenied);

        if (usbManager == null) {
            QGCLogger.e(TAG, "Failed to get UsbManager");
            enumerator = new UsbSerialEnumerator(null);
            enumerator.setStaleDriverCallback(this::closeAllPortsForDeviceName);
            return;
        }

        D2xxLibrary.initialize(appContext);
        enumerator = new UsbSerialEnumerator(buildProber());
        enumerator.setStaleDriverCallback(this::closeAllPortsForDeviceName);

        permissionManager.register(appContext);
        attachDetachReceiver.register(appContext);

        enumerator.updateCurrentDrivers(usbManager);
        for (final UsbDevice device : usbManager.getDeviceList().values()) {
            requestPermissionIfTracked(device);
        }
    }

    private void _destroy() {
        // Stop the FGS (idempotent) and reset configuredPortCount under serviceLock so a stale onPortConfigured during teardown can't re-trigger the start.
        synchronized (serviceLock) {
            configuredPortCount = 0;
            QGCForegroundService.stop(appContext);
        }
        for (final QGCSerialPort port : portRegistry.allPorts()) {
            port.close(QGCSerialPort.CloseReason.SHUTDOWN);
        }
        permissionManager.unregister();
        attachDetachReceiver.unregister();
        // Drain the worker before clearing the enumerator so an in-flight probe can't mutate the driver list after we wipe it.
        shutdownUsbWorker();
        D2xxLibrary.cleanup();
        enumerator.clear();
    }

    /**
     * Builds the {@link UsbSerialProber} for enumeration. Driver matching uses mik3y's default probe table
     * (CDC-ACM class-descriptor probing + built-in CP210x / FTDI / CH34x / Prolific VID/PID lists); the only
     * QGC-specific behavior is preferring the D2XX-backed {@link QGCFtdiSerialPort.QGCFtdiSerialDriver} when D2XX actually
     * enumerates the attached FTDI device.
     */
    private static UsbSerialProber buildProber() {
        return new UsbSerialProber(UsbSerialProber.getDefaultProbeTable()) {
            @Override
            public UsbSerialDriver probeDevice(final UsbDevice device) {
                if (device == null) return null;
                final String tag = String.format("vid=0x%04X pid=0x%04X (%s)",
                        device.getVendorId(), device.getProductId(), device.getDeviceName());
                // D2XX is only selected after the FTDI library itself recognizes the device; otherwise VCP-mode FtdiSerialDriver remains the fallback.
                if (D2xxLibrary.isAvailable() && D2xxLibrary.isFtdiDevice(device)
                        && D2xxLibrary.canOpenViaD2XX(device)) {
                    QGCLogger.i(TAG, "probe " + tag + " -> QGCFtdiSerialDriver (D2XX)");
                    return new QGCFtdiSerialPort.QGCFtdiSerialDriver(device);
                }
                // PL2303GT (0x067B/0x23C3) is handled correctly by upstream ProlificSerialDriver (descriptor-based DEVICE_TYPE_HXN skips doBlackMagic() since commit c82cd28, May 2021).
                final UsbSerialDriver driver = super.probeDevice(device);
                QGCLogger.i(TAG, "probe " + tag
                        + " -> " + (driver == null ? "no driver" : driver.getClass().getSimpleName()));
                return driver;
            }
        };
    }

    private void shutdownUsbWorker() {
        usbWorker.shutdown();
        try {
            if (!usbWorker.awaitTermination(USB_WORKER_DRAIN_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
                QGCLogger.w(TAG, "USB worker did not drain within " + USB_WORKER_DRAIN_TIMEOUT_MS + "ms; forcing shutdown");
                usbWorker.shutdownNow();
            }
        } catch (final InterruptedException e) {
            usbWorker.shutdownNow();
            Thread.currentThread().interrupt();
        }
    }

    private static final long USB_WORKER_DRAIN_TIMEOUT_MS = 2000L;

    /** Routes a JNI call to the live instance; returns fallback when not initialised. */
    private static <T> T withInstance(final T fallback, final Function<QGCUsbSerialManager, T> op) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return fallback;
        }
        return op.apply(mgr);
    }

    /**
     * Single-JNI-hop enumeration: serialises all ports into one UTF-8 JSON byte[] so C++ decodes with QJsonDocument
     * instead of N*7 getField crossings. Shape (must stay in lockstep with SerialPortInfoCodec::unpack):
     * {"ports":[{deviceName,productName,manufacturerName,serialNumber,productId,vendorId,baudRates:[...]}]}.
     * A null string field is omitted (distinct from an empty ""); a null/empty baud array yields [].
     */
    @UsedFromNativeCode
    public static byte[] availablePortsPacked() {
        return packPortsInfo(withInstance(new UsbPortInfo[0], QGCUsbSerialManager::_availablePortsInfo));
    }

    static byte[] packPortsInfo(final UsbPortInfo[] ports) {
        final JSONArray portsArray = new JSONArray();
        try {
            for (final UsbPortInfo port : ports) {
                final JSONObject obj = new JSONObject();
                putString(obj, SerialWireConstants.KEY_DEVICE_NAME, port.deviceName());
                putString(obj, SerialWireConstants.KEY_PRODUCT_NAME, port.productName());
                putString(obj, SerialWireConstants.KEY_MANUFACTURER_NAME, port.manufacturerName());
                putString(obj, SerialWireConstants.KEY_SERIAL_NUMBER, port.serialNumber());
                obj.put(SerialWireConstants.KEY_PRODUCT_ID, port.productId());
                obj.put(SerialWireConstants.KEY_VENDOR_ID, port.vendorId());
                final JSONArray baudArray = new JSONArray();
                final int[] bauds = port.supportedBaudRates();
                if (bauds != null) {
                    for (final int baud : bauds) {
                        baudArray.put(baud);
                    }
                }
                obj.put(SerialWireConstants.KEY_BAUD_RATES, baudArray);
                portsArray.put(obj);
            }
            final String json = new JSONObject().put(SerialWireConstants.KEY_PORTS, portsArray).toString();
            return json.getBytes(StandardCharsets.UTF_8);
        } catch (final JSONException e) {
            // org.json only throws on NaN/Infinity doubles, which this object graph never contains; treat as fatal.
            throw new IllegalStateException("Failed to pack UsbPortInfo", e);
        }
    }

    /** Omits the key for a null string so C++ can distinguish "absent" from an empty ""; org.json drops null puts. */
    private static void putString(final JSONObject obj, final String key, final String value) throws JSONException {
        if (value != null) {
            obj.put(key, value);
        }
    }

    /** One-shot open: register + configure + optionally start the reader in one JNI hop; null on failure (port rolled back). */
    @UsedFromNativeCode
    public static QGCSerialPort openConfiguredPort(final String deviceName, final long nativeHandle,
            final SerialParameters params, final int flowControl, final boolean assertDtr,
            final boolean startReader) {
        return withInstance(null, mgr -> {
            final QGCSerialPort port = mgr._openPort(deviceName, nativeHandle);
            if (port == null) {
                return null;
            }
            if (!port.configure(params, flowControl, assertDtr)) {
                return null;
            }
            if (startReader && !port.startIoManager()) {
                port.close(QGCSerialPort.CloseReason.OPEN_ROLLBACK);
                return null;
            }
            return port;
        });
    }

    private QGCSerialPort _openPort(final String deviceName, final long nativeHandle) {
        final UsbSerialEnumerator.DevicePortSpec spec = UsbSerialEnumerator.parseDevicePortSpec(deviceName);
        final UsbSerialDriver driver = enumerator.findDriverByDeviceName(spec.baseDeviceName());
        if (driver == null) {
            QGCLogger.d(TAG, "Attempt to open unknown device " + deviceName);
            return null;
        }

        final UsbSerialPort port = UsbSerialEnumerator.getPortFromDriver(driver, spec.portIndex());
        if (port == null) {
            QGCLogger.w(TAG, "No port " + spec.portIndex() + " available on device " + deviceName);
            return null;
        }

        final PortAddress address = new PortAddress(
                driver.getDevice().getDeviceName(),
                driver.getDevice().getDeviceId(),
                spec.portIndex());
        final QGCSerialPort[] holder = new QGCSerialPort[1];
        final QGCSerialPort serialPort = new QGCSerialPort(
                address,
                usbManager,
                driver,
                port,
                nativeHandle,
                this,
                () -> portRegistry.unregister(holder[0]));
        holder[0] = serialPort;

        if (!portRegistry.register(serialPort)) {
            final QGCSerialPort existing = portRegistry.get(address);
            final String state = (existing == null) ? "unknown" : existing.stateForLogging();
            QGCLogger.w(TAG, "Duplicate open rejected for " + address + " while existing port is " + state);
            return null;
        }
        return serialPort;
    }

    /** Immutable value-object carrying the four serial-port configuration fields. */
    @UsedFromNativeCode
    public record SerialParameters(int baudRate, int dataBits, int stopBits, int parity) {}

    /** Posts blocking USB enumeration onto the serial worker; drops the task if the worker is already shut down. */
    private void postToUsbWorker(final Runnable task) {
        try {
            usbWorker.execute(task);
        } catch (final RejectedExecutionException e) {
            QGCLogger.w(TAG, "USB worker rejected task (shutting down)");
        }
    }

    private void onUsbDeviceAttached(final UsbDevice device) {
        postToUsbWorker(() -> onDeviceAvailable(device));
    }

    private void onUsbDeviceDetached(final UsbDevice device) {
        postToUsbWorker(() -> {
            synchronized (deviceTeardownLock) {
                permissionManager.clearPermission(device.getDeviceName());
                for (final QGCSerialPort port : portRegistry.portsForDeviceName(device.getDeviceName())) {
                    port.close(QGCSerialPort.CloseReason.DETACHED);
                }
                QGCLogger.i(TAG, "Device detached: " + device.getDeviceName());
            }
            updateCurrentDrivers();   // probe outside the teardown lock
        });
    }

    private void onUsbPermissionGranted(final UsbDevice device) {
        postToUsbWorker(() -> onDeviceAvailable(device));
    }

    private void onUsbPermissionDenied(final UsbDevice device) {
        postToUsbWorker(() -> firePermissionDeniedForDeviceName(portRegistry, device.getDeviceName()));
    }

    static void firePermissionDeniedForDeviceName(final PortRegistry registry, final String deviceName) {
        for (final QGCSerialPort port : registry.portsForDeviceName(deviceName)) {
            port.fireException(SerialWireConstants.EXC_PERMISSION, "USB Permission Denied");
        }
    }

    @Override
    public void onPortConfigured(final QGCSerialPort port) {
        synchronized (serviceLock) {
            if (configuredPortCount++ == 0) {
                QGCForegroundService.start(appContext, USB_SERIAL_FGS_CONFIG);
            }
        }
    }

    @Override
    public void onPortClosed(final QGCSerialPort port) {
        synchronized (serviceLock) {
            if (configuredPortCount == 0) {
                QGCLogger.w(TAG, "onPortClosed with configuredPortCount==0 (counter skew)");
                return;
            }
            if (--configuredPortCount == 0) {
                QGCForegroundService.stop(appContext);
            }
        }
    }

    @Override
    public void onPortDeviceError(final QGCSerialPort port) {
        if (port == null || usbManager == null) {
            return;
        }
        final String deviceName = port.address().deviceName();
        // Source port already self-reports via fireException → close(); closing it again here would race a second nativeDeviceHasDisconnected against the in-flight exception lambda.
        synchronized (deviceTeardownLock) {
            for (final QGCSerialPort sibling : portRegistry.portsForDeviceName(deviceName)) {
                if (sibling != port) {
                    sibling.close(QGCSerialPort.CloseReason.DEVICE_ERROR);
                }
            }
        }
        // Probe + driver-list swap is enumerator-locked; running it outside deviceTeardownLock keeps a slow enumeration from stalling concurrent detach handlers.
        enumerator.replaceDriverForDeviceName(usbManager, deviceName);
    }

    /** Central funnel for cold-start enumeration, USB-attach, and permission-granted callbacks. */
    private void onDeviceAvailable(final UsbDevice device) {
        if (usbManager == null) {
            QGCLogger.w(TAG, "USB serial manager not ready, ignoring device " + device.getDeviceName());
            return;
        }
        enumerator.updateCurrentDrivers(usbManager);
        requestPermissionIfTracked(device);
        // Nudge the C++ autoconnect to re-scan immediately rather than waiting for its next poll tick.
        try {
            nativeUsbDevicesChanged();
        } catch (final UnsatisfiedLinkError e) {
            QGCLogger.w(TAG, "nativeUsbDevicesChanged unavailable", e);
        }
    }

    // Implemented in AndroidSerialPort.cc; registered against this class at JNI_OnLoad.
    private native void nativeUsbDevicesChanged();

    /** Caller must have already refreshed the tracked-driver set. */
    private void requestPermissionIfTracked(final UsbDevice device) {
        if (enumerator.findDriverByDeviceName(device.getDeviceName()) == null) {
            QGCLogger.d(TAG, "No driver found for device " + device.getDeviceName());
            return;
        }
        if (usbManager.hasPermission(device)) {
            return;
        }
        if (permissionManager.isPermissionDenied(device.getDeviceName())) {
            QGCLogger.d(TAG, "Skipping permission re-prompt for previously denied device "
                    + device.getDeviceName());
            return;
        }
        QGCLogger.i(TAG, "Requesting permission to use device " + device.getDeviceName());
        permissionManager.request(usbManager, device);
    }

    private void closeAllPortsForDeviceName(final String deviceName) {
        for (final QGCSerialPort port : portRegistry.portsForDeviceName(deviceName)) {
            port.close(QGCSerialPort.CloseReason.STALE_DRIVER);
        }
    }

    private void updateCurrentDrivers() {
        enumerator.updateCurrentDrivers(usbManager);
    }

    private UsbPortInfo[] _availablePortsInfo() {
        if (usbManager == null) {
            return new UsbPortInfo[0];
        }
        // Surfacing all tracked devices lets QGC see a second device hot-plugged while a link is active; duplicate-open is rejected in _openPort.
        return enumerator.availablePortsInfo();
    }
}
