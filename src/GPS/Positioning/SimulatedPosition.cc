#include "SimulatedPosition.h"

#include <algorithm>
#include <chrono>

#include <QtCore/QDateTime>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(SimulatedPositionLog, "GPS.PositionManager.SimulatedPosition")

SimulatedPosition::SimulatedPosition(QObject* parent, RuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _updateTask(_scheduler, this)
{
    qCDebug(SimulatedPositionLog) << this;

    _lastPosition.setTimestamp(QDateTime::currentDateTimeUtc());
    _lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488.));
    _lastPosition.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    _lastPosition.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1.0);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, kHeading);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, kHorizontalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, kVerticalVelocityMetersPerSec);
}

SimulatedPosition::~SimulatedPosition()
{
    qCDebug(SimulatedPositionLog) << this;
}

void SimulatedPosition::startUpdates()
{
    if (_updateTask.active()) {
        return;
    }
    _lastUpdateUs = _scheduler->nowUs();
    _scheduleUpdate();
}

void SimulatedPosition::stopUpdates()
{
    _updateTask.cancel();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit errorOccurred(QGeoPositionInfoSource::UpdateTimeoutError);
}

void SimulatedPosition::_scheduleUpdate()
{
    _updateTask.schedule(std::chrono::milliseconds((std::max) (updateInterval(), minimumUpdateInterval())),
                         [this]() { _updatePosition(); });
}

void SimulatedPosition::_updatePosition()
{
    const quint64 nowUs = _scheduler->nowUs();
    const auto elapsed = std::chrono::microseconds(nowUs - _lastUpdateUs);
    _lastUpdateUs = nowUs;
    const double seconds = std::chrono::duration<double>(elapsed).count();
    const auto coordinate = _lastPosition.coordinate();
    _lastPosition.setCoordinate(coordinate.atDistanceAndAzimuth(kHorizontalVelocityMetersPerSec * seconds, kHeading,
                                                                kVerticalVelocityMetersPerSec * seconds));
    _lastPosition.setTimestamp(QDateTime::currentDateTimeUtc());
    _scheduleUpdate();
    emit positionUpdated(_lastPosition);
}

void SimulatedPosition::setReferencePosition(const QGeoCoordinate& coordinate)
{
    if (coordinate.isValid()) {
        _lastPosition.setCoordinate(coordinate);
    }
}
