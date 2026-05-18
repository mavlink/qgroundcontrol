package org.mavlink.qgroundcontrol.serial;

/**
 * Structured device info record returned to C++ via JNI. Java records
 * auto-generate component accessors with the field name (e.g.
 * {@code deviceName()}, {@code productId()}) which JNI calls as
 * ordinary instance methods.
 */
public record UsbPortInfo(
        String deviceName,
        String productName,
        String manufacturerName,
        String serialNumber,
        int productId,
        int vendorId) {}
