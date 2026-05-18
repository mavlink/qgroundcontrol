package org.mavlink.qgroundcontrol.serial;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.util.SerialInputOutputManager;
import android.hardware.usb.UsbDeviceConnection;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.UnaryOperator;

/**
 * Maps JNI int handles to USB port resources. Single source of truth keyed by
 * handle; (physicalDeviceId, portIndex) dedup is enforced by a synchronized
 * register path.
 *
 * <p>Multi-field updates are atomic via {@link AtomicReference#updateAndGet} —
 * a reader always observes a self-consistent {@link UsbDeviceResources} snapshot.
 *
 * <p><b>Token invariant:</b> Java never holds a C++ pointer. {@code nativeToken}
 * is an opaque counter that the C++ JniPointerRegistry maps to an
 * {@code AndroidSerial*}; if the mapping is gone by the time Java calls back,
 * the callback is a no-op.
 */
final class UsbDeviceRegistry {

    /**
     * Per-handle lifecycle state. Transitions are linear:
     * {@code REGISTERED → OPEN → IO_READY → CLOSING}. {@link UsbSerialLifecycle}
     * owns transitions; callers query state instead of inferring it from
     * field-presence checks. {@link SerialInputOutputManager#getState()} is still
     * the authoritative IO running/stopped/stopping signal because the IO
     * thread can transition on its own (e.g., on read error).
     */
    enum PortLifecycleState {
        /** Placeholder allocated; no driver, port, or connection yet. */
        REGISTERED,
        /** Driver/port opened, UsbDeviceConnection (or D2XX-owned equivalent) live. */
        OPEN,
        /** SerialInputOutputManager constructed; ready to start, may already be running. */
        IO_READY,
        /** Teardown in progress; new operations should bail. */
        CLOSING,
    }

    /**
     * Immutable snapshot of all resources associated with a single open USB
     * serial port. Field updates produce new records via {@code with*}
     * copy-constructors and are applied atomically through the registry's
     * {@link AtomicReference}.
     */
    static record UsbDeviceResources(
            int handle,
            UsbSerialDriver driver,
            SerialInputOutputManager ioManager,
            QGCSerialListener listener,
            UsbDeviceConnection connection,
            AsyncUsbWritePump writePump,
            int fileDescriptor,
            long nativeToken,
            int portIndex,
            int physicalDeviceId,
            PortLifecycleState state) {

        UsbDeviceResources withDriver(UsbSerialDriver v) {
            return new UsbDeviceResources(handle, v, ioManager, listener, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withIoManager(SerialInputOutputManager v) {
            return new UsbDeviceResources(handle, driver, v, listener, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withListener(QGCSerialListener v) {
            return new UsbDeviceResources(handle, driver, ioManager, v, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withConnection(UsbDeviceConnection v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, v, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withWritePump(AsyncUsbWritePump v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, v,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withFileDescriptor(int v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, writePump,
                    v, nativeToken, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withNativeToken(long v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, writePump,
                    fileDescriptor, v, portIndex, physicalDeviceId, state);
        }

        UsbDeviceResources withPortIndex(int v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, writePump,
                    fileDescriptor, nativeToken, v, physicalDeviceId, state);
        }

        UsbDeviceResources withPhysicalDeviceId(int v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, v, state);
        }

        UsbDeviceResources withState(final PortLifecycleState v) {
            return new UsbDeviceResources(handle, driver, ioManager, listener, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, v);
        }

        /** Sets the five open-state fields and transitions to {@link PortLifecycleState#OPEN}
         *  in one record copy, cheaper under {@link AtomicReference#updateAndGet} retry. */
        UsbDeviceResources withOpenState(final UsbSerialDriver d, final int pi, final long token,
                final int fd, final UsbDeviceConnection c, final AsyncUsbWritePump pump) {
            return new UsbDeviceResources(handle, d, ioManager, listener, c, pump, fd, token, pi,
                    physicalDeviceId, PortLifecycleState.OPEN);
        }

        /** Sets the IO manager + listener and transitions to {@link PortLifecycleState#IO_READY}. */
        UsbDeviceResources withIoReady(final SerialInputOutputManager io, final QGCSerialListener l) {
            return new UsbDeviceResources(handle, driver, io, l, connection, writePump,
                    fileDescriptor, nativeToken, portIndex, physicalDeviceId, PortLifecycleState.IO_READY);
        }

        // handle=0 collides with BAD_DEVICE_ID — caller must pass the real handle.
        static UsbDeviceResources placeholder(final int handle, final int physicalDeviceId, final int portIndex) {
            return new UsbDeviceResources(
                    handle, null, null, null, null, null, 0, 0L,
                    portIndex, physicalDeviceId, PortLifecycleState.REGISTERED);
        }
    }

    /** Keyed by JNI handle. */
    private final ConcurrentHashMap<Integer, AtomicReference<UsbDeviceResources>> resources =
            new ConcurrentHashMap<>();

    private final AtomicInteger nextHandle = new AtomicInteger(1);

    /** Serializes register + remove against each other and against the slow-path
     *  dedup scan in {@link #getOrCreateHandle}. Read paths are lock-free. */
    private final Object registrationLock = new Object();

    /**
     * Returns the int handle for the given (physicalDeviceId, portIndex) pair.
     * If no entry exists, allocates a placeholder with a fresh handle. Concurrent
     * callers for the same pair converge to the same handle.
     */
    int getOrCreateHandle(final int physicalDeviceId, final int portIndex) {
        // Fast path — lock-free scan.
        UsbDeviceResources existing = findByPortKey(physicalDeviceId, portIndex);
        if (existing != null) {
            return existing.handle();
        }

        synchronized (registrationLock) {
            // Re-scan under lock; another thread may have registered concurrently.
            existing = findByPortKey(physicalDeviceId, portIndex);
            if (existing != null) {
                return existing.handle();
            }

            final int handle =
                    nextHandle.getAndUpdate(v -> (v >= Integer.MAX_VALUE) ? 1 : v + 1);
            final UsbDeviceResources res = UsbDeviceResources.placeholder(handle, physicalDeviceId, portIndex);
            resources.put(handle, new AtomicReference<>(res));
            return handle;
        }
    }

    /** Looks up a resource snapshot by JNI int handle. */
    UsbDeviceResources getByHandle(final int handle) {
        final AtomicReference<UsbDeviceResources> ref = resources.get(handle);
        return (ref != null) ? ref.get() : null;
    }

    /** Looks up a resource snapshot by (physicalDeviceId, portIndex) pair. */
    UsbDeviceResources getByPortKey(final int physicalDeviceId, final int portIndex) {
        return findByPortKey(physicalDeviceId, portIndex);
    }

    private UsbDeviceResources findByPortKey(final int physicalDeviceId, final int portIndex) {
        for (final AtomicReference<UsbDeviceResources> ref : resources.values()) {
            final UsbDeviceResources r = ref.get();
            if (r != null && r.physicalDeviceId() == physicalDeviceId && r.portIndex() == portIndex) {
                return r;
            }
        }
        return null;
    }

    /** Returns all handles whose entry has {@code physicalDeviceId}. */
    List<Integer> handlesForPhysicalDevice(final int physicalDeviceId) {
        final List<Integer> handles = new ArrayList<>();
        for (final AtomicReference<UsbDeviceResources> ref : resources.values()) {
            final UsbDeviceResources res = ref.get();
            if (res != null && res.physicalDeviceId() == physicalDeviceId) {
                handles.add(res.handle());
            }
        }
        return handles;
    }

    /**
     * Atomically applies {@code updater} to the current snapshot for
     * {@code handle}. Standard {@link AtomicReference#updateAndGet} retry on CAS
     * contention. No-op if no entry exists for {@code handle}.
     */
    void updateByHandle(final int handle, final UnaryOperator<UsbDeviceResources> updater) {
        final AtomicReference<UsbDeviceResources> ref = resources.get(handle);
        if (ref != null) {
            ref.updateAndGet(updater);
        }
    }

    /**
     * Atomically applies {@code updater} and returns the snapshot that was
     * replaced — the only safe way to extract a resource (e.g. owned
     * UsbDeviceConnection) for cleanup without racing another caller.
     * Returns {@code null} if no entry exists for {@code handle}.
     */
    UsbDeviceResources getAndUpdateByHandle(final int handle,
            final UnaryOperator<UsbDeviceResources> updater) {
        final AtomicReference<UsbDeviceResources> ref = resources.get(handle);
        return (ref != null) ? ref.getAndUpdate(updater) : null;
    }

    /**
     * Atomically nulls the connection field and returns the previous UsbDeviceResources
     * for the caller to close. Closing here would couple the registry to UsbDeviceConnection
     * lifecycle; callers handle close + log. Returns null if no entry, or the entry holds no connection.
     */
    UsbDeviceConnection extractConnection(final int handle) {
        final UsbDeviceResources prev = getAndUpdateByHandle(handle, r -> r.withConnection(null));
        return (prev != null) ? prev.connection() : null;
    }

    /** Atomically nulls the writePump field and returns the previous reference for caller cleanup. */
    AsyncUsbWritePump extractWritePump(final int handle) {
        final UsbDeviceResources prev = getAndUpdateByHandle(handle, r -> r.withWritePump(null));
        return (prev != null) ? prev.writePump() : null;
    }

    /** Removes the entry for {@code handle}. Returns the last snapshot, or null. */
    UsbDeviceResources remove(final int handle) {
        synchronized (registrationLock) {
            final AtomicReference<UsbDeviceResources> ref = resources.remove(handle);
            return (ref != null) ? ref.get() : null;
        }
    }

    /** Clears all entries and resets the handle counter. */
    void clear() {
        synchronized (registrationLock) {
            resources.clear();
            nextHandle.set(1);
        }
    }

    /** Returns a snapshot of all current resource entries. */
    List<UsbDeviceResources> allResources() {
        final List<UsbDeviceResources> snapshot = new ArrayList<>(resources.size());
        for (final AtomicReference<UsbDeviceResources> ref : resources.values()) {
            final UsbDeviceResources r = ref.get();
            if (r != null) {
                snapshot.add(r);
            }
        }
        return snapshot;
    }

}
