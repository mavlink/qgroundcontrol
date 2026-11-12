package org.mavlink.qgroundcontrol.serial;

/**
 * What happened to a USB device. The permission is the one that {@link QGCUsbSerialHost#requestPermission} asks
 * for.
 */
enum QGCUsbSerialEvent {
    ATTACHED,
    DETACHED,
    PERMISSION_GRANTED,
    PERMISSION_DENIED
}
