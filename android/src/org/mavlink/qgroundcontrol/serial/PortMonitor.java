package org.mavlink.qgroundcontrol.serial;

/** Shared seam onto the owning port for the read and write loops: lifecycle monitor, native handle, listener mute, and exception fan-out. */
interface PortMonitor {
    /** The owning port's lifecycle monitor; every {@code *Locked} query below must be made while holding it. */
    Object lock();
    /** Caller holds {@link #lock()}: snapshot of the live native handle (0 once closed). */
    long nativeHandleLocked();
    /** Caller holds {@link #lock()}: mark the listener muted so no stray emit/ack/exception reaches a torn-down native side. */
    void muteListenerLocked();
    void fireException(int kind, String message);
}
