#pragma once

#include <QtCore/QtTypes>

class QString;

namespace QGC
{
    float limitAngleToPMPIf(double angle);
    double limitAngleToPMPId(double angle);

    /// Returns true if the two values are equal or close. Correctly handles 0 and NaN values.
    bool fuzzyCompare(double value1, double value2);
    bool fuzzyCompare(float value1, float value2);
    bool fuzzyCompare(double value1, double value2, double tolerance);
    bool fuzzyCompare(float value1, float value2, float tolerance);

    quint32 crc32(const quint8 *src, unsigned len, unsigned state);

    // ---- Locale-aware number/size formatting ----------------------------------
    // Free functions so callers avoid pulling QApplication through QGCApplication.h.

    /// Locale-aware decimal integer formatting (e.g. "1,234,567").
    QString numberToString(quint64 number);

    /// Locale-aware byte-size with unit: B, KB, MB, GB, TB. 1 fractional digit above 1 KB.
    QString bigSizeToString(quint64 size);

    /// Locale-aware size scaled to MB, GB, or TB. Input is in MB.
    QString bigSizeMBToString(quint64 sizeMB);

    // ---- Application message helpers ------------------------------------------
    // Free-function wrappers over QGCApplication's message API. Safe to call
    // before qgcApp() exists (no-ops).

    /// Show a modal application message. Queued if the UI isn't ready yet.
    void showAppMessage(const QString &message, const QString &title = QString());

    /// Show a non-modal vehicle message. PreArm/preflight messages are routed
    /// through the Vehicle path and suppressed here.
    void showCriticalVehicleMessage(const QString &message);

    /// Show a modal reboot-required message. Debounced within 2 minutes.
    void showRebootAppMessage(const QString &message, const QString &title = QString());

    /// True if the app is running in unit-test mode. Returns false if qgcApp() doesn't exist yet.
    bool runningUnitTests();
}
