#include "GPSPositionSourceRegistration.h"

#include "PositionManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionSourceRegistrationLog, "GPS.PositionManager.GPSPositionSourceRegistration")

GPSPositionSourceRegistration::GPSPositionSourceRegistration(QGCPositionManager* manager, int kind, quint64 token)
    : _manager(manager)
    , _kind(kind)
    , _token(token)
{
    qCDebug(GPSPositionSourceRegistrationLog) << this;
}

GPSPositionSourceRegistration::~GPSPositionSourceRegistration()
{
    qCDebug(GPSPositionSourceRegistrationLog) << this;
    if (_manager) {
        _manager->_retireRegistration(_kind, _token);
    }
}
