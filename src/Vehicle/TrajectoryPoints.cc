/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TrajectoryPoints.h"
#include "Vehicle.h"

TrajectoryPoints::TrajectoryPoints(Vehicle* vehicle, QObject* parent)
    : QObject       (parent)
    , _vehicle      (vehicle)
    , _lastAzimuth  (qQNaN())
{
}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate)
{
    if (_lastPoint.isValid()) {
        if (_lastPoint.distanceTo(coordinate) > _distanceTolerance) {
            double newAzimuth = _lastPoint.azimuthTo(coordinate);
            if (qIsNaN(_lastAzimuth) || qAbs(newAzimuth - _lastAzimuth) > _azimuthTolerance) {
                _lastAzimuth = _lastPoint.azimuthTo(coordinate);
                _lastPoint = coordinate;
                _points.append(QVariant::fromValue(coordinate));
                emit pointAdded(coordinate);
            } else {
                _lastPoint = coordinate;
                _points[_points.count() - 1] = QVariant::fromValue(coordinate);
                emit updateLastPoint(coordinate);
            }
        }
    } else {
        _lastAzimuth = _lastPoint.azimuthTo(coordinate);
        _lastPoint = coordinate;
        _points.append(QVariant::fromValue(coordinate));
        emit pointAdded(coordinate);
    }
}

void TrajectoryPoints::start(void)
{
    clear();
    connect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::stop(void)
{
    qDebug() << "Stop" << _points.count();
    disconnect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::clear(void)
{
    _points.clear();
    emit pointsCleared();
}
