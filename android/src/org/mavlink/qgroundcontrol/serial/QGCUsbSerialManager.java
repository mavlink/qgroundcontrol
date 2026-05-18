package org.mavlink.qgroundcontrol.serial;

import static org.mavlink.qgroundcontrol.serial.SerialConstants.BAD_DEVICE_ID;
import static org.mavlink.qgroundcontrol.serial.SerialConstants.MAX_NATIVE_CALLBACK_DATA_BYTES;

import org.mavlink.qgroundcontrol.QGCLogger;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.SystemClock;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Function;

/**
 * Java never holds a C++ pointer. {@code nativeToken} is an opaque random {@code long}
 * that the C++ JniPointerRegistry maps to an {@code AndroidSerial*}; if the mapping is
 * gone by the time Java calls back, the callback is a no-op. Token-based UAF protection,
 * identical to Qt Bluetooth LowEnergyNotificationHub.
 */
public class QGCUsbSerialManager
        implements QGCUsbPermissionHandler.Listener, UsbSerialLifecycle.Listener {

    private static final String TAG = QGCUsbSerialManager.class.getSimpleName();

    private final NativeDataEmitter nativeDataEmitter =
            new NativeDataEmitter(QGCUsbSerialManager::nativeDeviceNewData);

    private static final Object lifecycleLock = new Object();
    private static volatile QGCUsbSerialManager sInstance;

    /**
     * Creates the singleton.  No-op if already created.
     * Should be called once from {@code QGCActivity.onCreate()}.
     */
    public static void createInstance(final Context context) {
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
    public static void destroyInstance() {
        synchronized (lifecycleLock) {
            if (sInstance == null) {
                return;
            }
            sInstance._destroy();
            sInstance = null;
        }
    }

    private final UsbManager usbManager;
    private final Context appContext;
    private final QGCUsbPermissionHandler permissionHandler;
    private volatile NativeCallbacks nativeCallbacks = new JniNativeCallbacks();

    private final UsbSerialEnumerator enumerator;
    private final UsbSerialIoBridge ioBridge;
    private final UsbSerialLifecycle lifecycle;
    private final UsbDeviceRegistry deviceRegistry = new UsbDeviceRegistry();

    private QGCUsbSerialManager(final Context context) {
        appContext = context.getApplicationContext();
        usbManager = (UsbManager) appContext.getSystemService(Context.USB_SERVICE);
        permissionHandler = new QGCUsbPermissionHandler(this);

        if (usbManager == null) {
            QGCLogger.e(TAG, "Failed to get UsbManager");
            enumerator = new UsbSerialEnumerator(null, this::removeAllResourcesForPhysicalDevice);
            lifecycle = new UsbSerialLifecycle(null, deviceRegistry, enumerator, this);
            ioBridge = buildIoBridge();
            return;
        }

        QGCFtdiSerialDriver.initialize(appContext);
        enumerator = new UsbSerialEnumerator(
                QGCUsbSerialProber.getQGCUsbSerialProber(),
                this::removeAllResourcesForPhysicalDevice);
        lifecycle = new UsbSerialLifecycle(usbManager, deviceRegistry, enumerator, this);
        ioBridge = buildIoBridge();

        permissionHandler.register(appContext);

        enumerator.updateCurrentDrivers(usbManager);
        for (final UsbDevice device : usbManager.getDeviceList().values()) {
            requestPermissionIfTracked(device);
        }
    }

    private UsbSerialIoBridge buildIoBridge() {
        return new UsbSerialIoBridge(new UsbSerialIoBridge.PortResolver() {
            @Override
            public UsbSerialPort openPortOrWarn(final int deviceId, final String operation) {
                final UsbSerialPort port = lifecycle.findPortByDeviceId(deviceId);
                if (port == null) {
                    // Debug: null is the normal detach→drain race (findPortByDeviceId already logged).
                    QGCLogger.d(TAG, "Attempted to " + operation
                            + " on a null port for device ID " + deviceId);
                    return null;
                }
                if (!port.isOpen()) {
                    QGCLogger.d(TAG, "Attempted to " + operation
                            + " on a closed port for device ID " + deviceId);
                    return null;
                }
                return port;
            }
            @Override
            public SerialInputOutputManager ioManager(final int deviceId) {
                final UsbDeviceRegistry.UsbDeviceResources res = deviceRegistry.getByHandle(deviceId);
                return (res != null) ? res.ioManager() : null;
            }
            @Override
            public AsyncUsbWritePump writePump(final int deviceId) {
                final UsbDeviceRegistry.UsbDeviceResources res = deviceRegistry.getByHandle(deviceId);
                return (res != null) ? res.writePump() : null;
            }
        });
    }

    private void _destroy() {
        for (final UsbDeviceRegistry.UsbDeviceResources res :
                new ArrayList<>(deviceRegistry.allResources())) {
            lifecycle.close(res.handle());
        }

        permissionHandler.unregister();
        QGCFtdiSerialDriver.cleanup();
        enumerator.clear();
        deviceRegistry.clear();
    }

    /** Routes a JNI call to the live instance; returns fallback when not initialised.
     *  JNI binds by name and signature — do not rename or change arg types without updating AndroidSerial.cc. */
    private static <T> T withInstance(final T fallback, final Function<QGCUsbSerialManager, T> op) {
        final QGCUsbSerialManager mgr = getInstance();
        if (mgr == null) {
            QGCLogger.e(TAG, "Manager not initialized");
            return fallback;
        }
        return op.apply(mgr);
    }

    public static int getDeviceHandle(final int deviceId) {
        return withInstance(-1, m -> m._getDeviceHandle(deviceId));
    }

    /** Returns an empty array (never null) when the manager is not yet
     *  initialised, so callers need no null check. */
    public static UsbPortInfo[] availablePortsInfo() {
        return withInstance(new UsbPortInfo[0], QGCUsbSerialManager::_availablePortsInfo);
    }

    /**
     * Opens the device and applies serial parameters, flow control, and DTR in a single
     * JNI roundtrip. Returns the device ID on success, BAD_DEVICE_ID on any failure (after
     * which the port is fully closed and unregistered — no need for the C++ side to clean up).
     */
    public static int openWithConfig(final String deviceName, final SerialParameters params,
            final int flowControl, final boolean assertDtr, final long nativeToken) {
        return withInstance(BAD_DEVICE_ID, mgr -> {
            final int deviceId = mgr.lifecycle.open(deviceName, nativeToken);
            if (deviceId == BAD_DEVICE_ID) {
                return BAD_DEVICE_ID;
            }
            if (!mgr.ioBridge.setParameters(deviceId, params.baudRate(), params.dataBits(),
                            params.stopBits(), params.parity())
                    || !mgr.ioBridge.setFlowControl(deviceId, flowControl)
                    || (assertDtr && !mgr.ioBridge.setControlLine(deviceId, UsbSerialPort.ControlLine.DTR, true))) {
                QGCLogger.e(TAG, "Failed to apply post-open config for " + deviceName + "; rolling back");
                mgr.lifecycle.close(deviceId);
                return BAD_DEVICE_ID;
            }
            return deviceId;
        });
    }

    public static boolean startIoManager(final int deviceId) {
        return withInstance(false, m -> m.lifecycle.startIoManager(deviceId));
    }

    public static boolean stopIoManager(final int deviceId) {
        return withInstance(false, m -> m.lifecycle.stopIoManager(deviceId));
    }

    public static boolean ioManagerRunning(final int deviceId) {
        return withInstance(false, m -> m.lifecycle.ioManagerRunning(deviceId));
    }

    public static boolean close(final int deviceId) {
        return withInstance(false, m -> m.lifecycle.close(deviceId));
    }

    // TEMP: write-rate counter. Logs writes-per-5s every 5s on Android.Serial. Remove once
    // pooling decision is made (see qserialport_android.cpp design notes).
    private static final AtomicLong sWriteCount = new AtomicLong();
    private static volatile long sWriteWindowStartMs = 0;

    private static void countWrite() {
        sWriteCount.incrementAndGet();
        final long now = SystemClock.elapsedRealtime();
        final long windowStart = sWriteWindowStartMs;
        if (now - windowStart >= 5000) {
            // Best-effort: a single thread wins the CAS; others append into the next window.
            if (sWriteWindowStartMs == windowStart) {
                sWriteWindowStartMs = now;
                final long n = sWriteCount.getAndSet(0);
                QGCLogger.i(TAG, "writes/5s=" + n + " (≈" + (n / 5) + "/s, ≈" + (n * 2 / 5) + " java-allocs/s)");
            }
        }
    }

    /**
     * Zero-copy entry point from JNI. The C++ caller wraps its own buffer with
     * {@code NewDirectByteBuffer} — no {@code NewByteArray}/{@code SetByteArrayRegion}
     * copy occurs on the C++ side. One copy remains here (direct ByteBuffer → Java
     * heap byte[]) because usb-serial-for-android's upstream API takes byte[]; the
     * C++-side NewByteArray + SetByteArrayRegion copy is eliminated.
     */
    public static int writeDirect(final int deviceId, final ByteBuffer data, final int length,
            final int timeoutMSec) {
        countWrite();
        return withInstance(-1, m -> m.ioBridge.writeDirect(deviceId, data, length, timeoutMSec));
    }

    /** Async variant of {@link #writeDirect}. Same one-copy contract; byte[] is freshly allocated because the IO manager retains the reference. */
    public static int writeAsyncDirect(final int deviceId, final ByteBuffer data, final int length,
            final int timeoutMSec) {
        countWrite();
        return withInstance(-1, m -> m.ioBridge.writeAsyncDirect(deviceId, data, length, timeoutMSec));
    }

    /** Immutable value-object carrying the four serial-port configuration fields. */
    public record SerialParameters(int baudRate, int dataBits, int stopBits, int parity) {}

    public static boolean setSerialParameters(final int deviceId, final SerialParameters params) {
        return withInstance(false, m -> m.ioBridge.setParameters(deviceId,
                params.baudRate(), params.dataBits(), params.stopBits(), params.parity()));
    }

    public static boolean setDataTerminalReady(final int deviceId, final boolean on) {
        return withInstance(false, m -> m.ioBridge.setControlLine(deviceId, UsbSerialPort.ControlLine.DTR, on));
    }

    public static boolean setRequestToSend(final int deviceId, final boolean on) {
        return withInstance(false, m -> m.ioBridge.setControlLine(deviceId, UsbSerialPort.ControlLine.RTS, on));
    }

    public static int[] getControlLines(final int deviceId) {
        return withInstance(new int[]{}, m -> m.ioBridge.getControlLines(deviceId));
    }

    public static int getFlowControl(final int deviceId) {
        return withInstance(0, m -> m.ioBridge.getFlowControl(deviceId));
    }

    public static boolean setFlowControl(final int deviceId, final int flowControl) {
        return withInstance(false, m -> m.ioBridge.setFlowControl(deviceId, flowControl));
    }

    public static boolean setBreak(final int deviceId, final boolean on) {
        return withInstance(false, m -> m.ioBridge.setBreak(deviceId, on));
    }

    public static boolean purgeBuffers(final int deviceId, final boolean input, final boolean output) {
        return withInstance(false, m -> m.ioBridge.purgeBuffers(deviceId, input, output));
    }

    @Override
    public void onUsbDeviceAttached(final UsbDevice device) {
        onDeviceAvailable(device);
    }

    @Override
    public void onUsbDeviceDetached(final UsbDevice device) {
        final int physicalDeviceId = device.getDeviceId();
        permissionHandler.clearDevice(physicalDeviceId);
        final List<Integer> resourceIds = deviceRegistry.handlesForPhysicalDevice(physicalDeviceId);
        for (final Integer resourceId : resourceIds) {
            final UsbDeviceRegistry.UsbDeviceResources res = deviceRegistry.getByHandle(resourceId);
            if (res == null) {
                continue;
            }
            final long nativeToken = res.nativeToken();
            lifecycle.close(resourceId);
            emitDeviceHasDisconnected(nativeToken);
        }
        QGCLogger.i(TAG, "Device detached: " + device.getDeviceName());
        updateCurrentDrivers();
    }

    @Override
    public void onUsbPermissionGranted(final UsbDevice device) {
        onDeviceAvailable(device);
    }

    @Override
    public void onUsbPermissionDenied(final UsbDevice device) {
        final int physicalDeviceId = device.getDeviceId();
        for (final Integer resourceId : deviceRegistry.handlesForPhysicalDevice(physicalDeviceId)) {
            final UsbDeviceRegistry.UsbDeviceResources res = deviceRegistry.getByHandle(resourceId);
            if (res != null) {
                emitDeviceException(res.nativeToken(), SerialConstants.EXC_PERMISSION,
                        "USB Permission Denied");
            }
        }
    }

    interface NativeCallbacks {
        void onDeviceHasDisconnected(long nativeToken);
        void onDeviceException(long nativeToken, int kind, String message);
    }

    private static final class JniNativeCallbacks implements NativeCallbacks {
        @Override
        public void onDeviceHasDisconnected(final long nativeToken) {
            nativeDeviceHasDisconnected(nativeToken);
        }

        @Override
        public void onDeviceException(final long nativeToken, final int kind, final String message) {
            nativeDeviceException(nativeToken, kind, message);
        }
    }

    // JNI bridge — bound by name from AndroidSerial.cc setNativeMethods().
    private static native void nativeDeviceHasDisconnected(final long nativeToken);
    private static native void nativeDeviceException(final long nativeToken, final int kind, final String message);
    /**
     * JNI bridge — accepts a direct ByteBuffer so C++ can use GetDirectBufferAddress
     * (zero-copy at the JNI boundary). {@code length} is the number of valid bytes
     * in the buffer; capacity may be larger because the buffer comes from a pool.
     *
     * <p><b>Contract:</b> the native side MUST consume {@code data} synchronously
     * before returning. The buffer is recycled into the {@link NativeDataEmitter}
     * pool as soon as this call returns; retaining the pointer past return is a
     * use-after-free on off-heap memory. Bytes may be appended to a Java/owner-side
     * queue inside the JNI call, but the {@link ByteBuffer} itself must not be stored.</p>
     */
    private static native void nativeDeviceNewData(final long nativeToken, final ByteBuffer data, final int length);

    @Override
    public void emitException(final long nativeToken, final int kind, final String message) {
        emitDeviceException(nativeToken, kind, message);
    }

    @Override
    public void emitNewData(final long nativeToken, final byte[] data, final int offset, final int length) {
        emitDeviceNewData(nativeToken, data, offset, length);
    }

    private void emitDeviceHasDisconnected(final long nativeToken) {
        if (nativeToken != 0) {
            nativeCallbacks.onDeviceHasDisconnected(nativeToken);
        }
    }

    private void emitDeviceException(final long nativeToken, final int kind, final String message) {
        if (nativeToken != 0) {
            nativeCallbacks.onDeviceException(nativeToken, kind, message);
        }
    }

    private void emitDeviceNewData(final long nativeToken, final byte[] data, final int offset, final int length) {
        if (nativeToken == 0 || data == null || length <= 0) {
            return;
        }
        // Payload exceeding this size bypassed QGCSerialListener's chunking and would overflow the pooled direct buffer.
        if (length > MAX_NATIVE_CALLBACK_DATA_BYTES) {
            throw new IllegalArgumentException("emitDeviceNewData length " + length
                    + " exceeds MAX_NATIVE_CALLBACK_DATA_BYTES " + MAX_NATIVE_CALLBACK_DATA_BYTES);
        }
        nativeDataEmitter.emit(nativeToken, data, offset, length);
    }

    /** Central funnel for cold-start enumeration, USB-attach, and permission-granted callbacks. */
    private void onDeviceAvailable(final UsbDevice device) {
        if (usbManager == null) {
            QGCLogger.w(TAG, "USB serial manager not ready, ignoring device " + device.getDeviceName());
            return;
        }

        enumerator.updateCurrentDrivers(usbManager);
        requestPermissionIfTracked(device);
    }

    /** Caller must have already refreshed the tracked-driver set via {@link UsbSerialEnumerator#updateCurrentDrivers}. */
    private void requestPermissionIfTracked(final UsbDevice device) {
        if (enumerator.findDriverByDeviceId(device.getDeviceId()) == null) {
            QGCLogger.d(TAG, "No driver found for device " + device.getDeviceName());
            return;
        }

        if (usbManager.hasPermission(device)) {
            return;
        }
        if (permissionHandler.isDenied(device.getDeviceId())) {
            QGCLogger.d(TAG, "Skipping permission re-prompt for previously denied device "
                    + device.getDeviceName());
            return;
        }
        QGCLogger.i(TAG, "Requesting permission to use device " + device.getDeviceName());
        permissionHandler.requestPermission(usbManager, device);
    }

    private void updateCurrentDrivers() {
        enumerator.updateCurrentDrivers(usbManager);
    }

    /** StaleDriverCallback target (method ref bound at constructor time, before {@code lifecycle} is assigned). */
    private void removeAllResourcesForPhysicalDevice(final int physicalDeviceId) {
        lifecycle.removeAllResourcesForPhysicalDevice(physicalDeviceId);
    }

    private int _getDeviceHandle(final int deviceId) {
        final UsbDeviceRegistry.UsbDeviceResources res = deviceRegistry.getByHandle(deviceId);
        return (res != null) ? res.fileDescriptor() : -1;
    }

    private UsbPortInfo[] _availablePortsInfo() {
        if (usbManager == null) {
            return new UsbPortInfo[0];
        }
        return enumerator.availablePortsInfo();
    }

}
