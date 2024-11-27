/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SimulatedPosition.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(SimulatedPositionLog, "qgc.positionmanager.simulatedposition")

SimulatedPosition::SimulatedPosition(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _updateTimer(new QTimer(this))
{
    // qCDebug(SimulatedPositionLog) << Q_FUNC_INFO << this;

    _lastPosition.setTimestamp(QDateTime::currentDateTime());
    _lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488.));
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, kHeading);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, kHorizontalVelocityMetersPerSec);
    _lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, kVerticalVelocityMetersPerSec);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &SimulatedPosition::_vehicleAdded);

    _updateTimer->setSingleShot(false);
    (void) connect(_updateTimer, &QTimer::timeout, this, &SimulatedPosition::_updatePosition);
}

SimulatedPosition::~SimulatedPosition()
{
    // qCDebug(SimulatedPositionLog) << Q_FUNC_INFO << this;
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
