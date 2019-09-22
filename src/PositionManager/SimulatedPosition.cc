/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore>
#include <QDateTime>
#include <QDate>

#include "SimulatedPosition.h"

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

    connect(&_updateTimer, &QTimer::timeout, this, &SimulatedPosition::updatePosition);
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

void SimulatedPosition::updatePosition(void)
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


