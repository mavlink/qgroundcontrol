#include "GPSPositionSourceRegistration.h"

#include <utility>

#include "PositionManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionSourceRegistrationLog, "GPS.PositionManager.GPSPositionSourceRegistration")

GPSPositionSourceRegistration::GPSPositionSourceRegistration()
{
    qCDebug(GPSPositionSourceRegistrationLog) << this;
}

GPSPositionSourceRegistration::GPSPositionSourceRegistration(GPSPositionSourceRegistration&& other) noexcept
{
    qCDebug(GPSPositionSourceRegistrationLog) << this;
    _swap(other);
}

GPSPositionSourceRegistration& GPSPositionSourceRegistration::operator=(GPSPositionSourceRegistration&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    GPSPositionSourceRegistration retired(std::move(*this));
    _swap(other);
    return *this;
}

void GPSPositionSourceRegistration::_swap(GPSPositionSourceRegistration& other) noexcept
{
    _manager.swap(other._manager);
    std::swap(_kind, other._kind);
    std::swap(_token, other._token);
}

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
    reset();
}

void GPSPositionSourceRegistration::reset()
{
    const auto manager = std::exchange(_manager, {});
    const int kind = std::exchange(_kind, 0);
    const quint64 token = std::exchange(_token, 0);
    if (manager && token) {
        manager->_retireRegistration(kind, token);
    }
}
