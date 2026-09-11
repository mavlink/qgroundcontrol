#include "SimulatedPosition.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(SimulatedPositionLog, "GPS.PositionManager.SimulatedPosition")

SimulatedPosition::SimulatedPosition(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _updateTimer(new QTimer(this))
{
    qCDebug(SimulatedPositionLog) << this;

    _lastPosition.setTimestamp(QDateTime::currentDateTime());
    _lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488.));
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, kHeading);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, kHorizontalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, kVerticalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::HorizontalAccuracy, 1.0);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalAccuracy, 1.0);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &SimulatedPosition::_vehicleAdded);

    _updateTimer->setSingleShot(false);
    (void) connect(_updateTimer, &QTimer::timeout, this, &SimulatedPosition::_updatePosition);
}

SimulatedPosition::~SimulatedPosition()
{
    qCDebug(SimulatedPositionLog) << this;
}

void SimulatedPosition::startUpdates()
{
    _updateTimer->start(qMax(updateInterval(), minimumUpdateInterval()));
}

void SimulatedPosition::stopUpdates()
{
    _updateTimer->stop();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit errorOccurred(QGeoPositionInfoSource::UpdateTimeoutError);
}

void SimulatedPosition::_updatePosition()
{
    const int intervalMsecs = _updateTimer->interval();

    const QGeoCoordinate coord = _lastPosition.coordinate();
    const qreal horizontalDistance = kHorizontalVelocityMetersPerSec * (1000. / static_cast<qreal>(intervalMsecs));
    const qreal verticalDistance = kVerticalVelocityMetersPerSec * (1000. / static_cast<qreal>(intervalMsecs));

    _lastPosition.setCoordinate(coord.atDistanceAndAzimuth(horizontalDistance, kHeading, verticalDistance));
    _lastPosition.setTimestamp(QDateTime::currentDateTimeUtc());
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
