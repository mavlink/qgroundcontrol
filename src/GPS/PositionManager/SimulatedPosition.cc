#include "SimulatedPosition.h"

#include <QtCore/QDateTime>

#include <algorithm>
#include <chrono>

#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(SimulatedPositionLog, "GPS.PositionManager.SimulatedPosition")

SimulatedPosition::SimulatedPosition(QObject* parent, RuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent),
      _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)),
      _updateTask(_scheduler, this)
{
    qCDebug(SimulatedPositionLog) << this;

    _lastPosition.setTimestamp(QDateTime::currentDateTimeUtc());
    _lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488.));
    _lastPosition.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    _lastPosition.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1.0);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, kHeading);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, kHorizontalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, kVerticalVelocityMetersPerSec);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &SimulatedPosition::_vehicleAdded);

    if (_scheduler->thread() != thread()) {
        qCWarning(SimulatedPositionLog) << "Scheduler must share the simulated source thread";
        _scheduler = nullptr;
    }
}

SimulatedPosition::~SimulatedPosition()
{
    qCDebug(SimulatedPositionLog) << this;
}

void SimulatedPosition::startUpdates()
{
    if (!_scheduler || _updateTask.active()) {
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
    if (!_scheduler) {
        return;
    }
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

void SimulatedPosition::_vehicleAdded(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    if (vehicle->homePosition().isValid()) {
        _lastPosition.setCoordinate(vehicle->homePosition());
    } else {
        _homePositionChangedConnection = connect(vehicle, &Vehicle::homePositionChanged, this, &SimulatedPosition::_vehicleHomePositionChanged);
    }
}

void SimulatedPosition::_vehicleHomePositionChanged(QGeoCoordinate homePosition)
{
    if (homePosition.isValid()) {
        _lastPosition.setCoordinate(homePosition);
        if (_homePositionChangedConnection) {
            (void) disconnect(_homePositionChangedConnection);
            _homePositionChangedConnection = QMetaObject::Connection();
        }
    }
}
