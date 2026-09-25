#include "GPSNotificationQueue.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSNotificationQueueLog, "GPS.Core.GPSNotificationQueue")

void GPSNotificationQueue::_deliver()
{
    if (_delivering) {
        return;
    }
    // The queue is a member of its owner, so nothing of it may be read once the owner is gone.
    const QPointer<QObject> owner(_owner);
    const char* const ownerClass = _ownerClass;
    _delivering = true;
    while (!_pending.empty()) {
        auto notification = std::move(_pending.front().second);
        _pending.erase(_pending.begin());
        notification();
        if (!owner) {
            qCWarning(GPSNotificationQueueLog)
                << ownerClass << "was deleted by its own change notification; observers must use deleteLater()";
            return;
        }
    }
    _delivering = false;
}
