#pragma once

#include <QtCore/QString>

// No-ops if called before qgcApp() exists.
namespace QGC
{
    /// Modal application message. Queued if the UI isn't ready yet.
    void showAppMessage(const QString &message, const QString &title = QString());

    /// Non-modal vehicle message. PreArm/preflight messages are suppressed
    /// here; they're routed through the Vehicle path instead.
    void showCriticalVehicleMessage(const QString &message);

    /// Modal reboot-required message. Debounced within 2 minutes.
    void showRebootAppMessage(const QString &message, const QString &title = QString());

    bool runningUnitTests();
}
