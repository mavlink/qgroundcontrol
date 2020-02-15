/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore>
#include <QDateTime>
#include <QDate>

#include "SimulatedPosition.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

SimulatedPosition::SimulatedPosition()
    : QGeoPositionInfoSource(nullptr)
{
    _updateTimer.setSingleShot(false);

    // Initialize position to normal PX4 Gazebo home position
    _lastPosition.setTimestamp(QDateTime::currentDateTime());
    _lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488));
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, _heading);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, _horizontalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, _verticalVelocityMetersPerSec);

    // When a vehicle shows up we switch location to the vehicle home position
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &SimulatedPosition::_vehicleAdded);

    connect(&_updateTimer, &QTimer::timeout, this, &SimulatedPosition::_updatePosition);
}

QGeoPositionInfo SimulatedPosition::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return _lastPosition;
}

SimulatedPosition::PositioningMethods SimulatedPosition::supportedPositioningMethods() const
{
    return AllPositioningMethods;
}

void SimulatedPosition::startUpdates(void)
{
    _updateTimer.start(qMax(updateInterval(), minimumUpdateInterval()));
}

void SimulatedPosition::stopUpdates(void)
{
    _updateTimer.stop();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit updateTimeout();
}

void SimulatedPosition::_updatePosition(void)
{
    int intervalMsecs = _updateTimer.interval();

    QGeoCoordinate  coord =                 _lastPosition.coordinate();
    double          horizontalDistance =    _horizontalVelocityMetersPerSec * (1000.0 / static_cast<double>(intervalMsecs));
    double          verticalDistance =      _verticalVelocityMetersPerSec * (1000.0 / static_cast<double>(intervalMsecs));

    _lastPosition.setCoordinate(coord.atDistanceAndAzimuth(horizontalDistance, _heading, verticalDistance));

    emit positionUpdated(_lastPosition);
}

QGeoPositionInfoSource::Error SimulatedPosition::error() const
{
    return QGeoPositionInfoSource::NoError;
}

void SimulatedPosition::_vehicleAdded(Vehicle* vehicle)
{
    if (vehicle->homePosition().isValid()) {
        _lastPosition.setCoordinate(vehicle->homePosition());
    } else {
        connect(vehicle, &Vehicle::homePositionChanged, this, &SimulatedPosition::_vehicleHomePositionChanged);
    }
}

void SimulatedPosition::_vehicleHomePositionChanged(QGeoCoordinate homePosition)
{
    Vehicle* vehicle = qobject_cast<Vehicle*>(sender());

    if (homePosition.isValid()) {
        _lastPosition.setCoordinate(homePosition);
        disconnect(vehicle, &Vehicle::homePositionChanged, this, &SimulatedPosition::_vehicleHomePositionChanged);
    }
}
