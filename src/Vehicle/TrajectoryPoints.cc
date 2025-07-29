/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TrajectoryPoints.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(TrajectoryPointsLog, "qgc.vehicle.trajectorypoints")

TrajectoryPoints::TrajectoryPoints(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    qCDebug(TrajectoryPointsLog) << this;
}

TrajectoryPoints::~TrajectoryPoints()
{
    qCDebug(TrajectoryPointsLog) << this;
}

void TrajectoryPoints::start()
{
    clear();
    (void) connect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::stop()
{
    (void) disconnect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::clear()
{
    _path.clearPath();
    _lastPoint = QGeoCoordinate();
    _lastAzimuth = qQNaN();
    emit pathChanged(_path);
}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate)
{
    if (_lastPoint.isValid()) {
        double distance = _lastPoint.distanceTo(coordinate);
        if (distance > kDistanceTolerance) {
            // Update total flight distance maintained by Vehicle
            _vehicle->updateFlightDistance(distance);

            const double newAzimuth = _lastPoint.azimuthTo(coordinate);
            if (qIsNaN(_lastAzimuth) || (qAbs(newAzimuth - _lastAzimuth) > kAzimuthTolerance)) {
                // Not colinear — append a new segment vertex
                _lastAzimuth = newAzimuth;
                _path.addCoordinate(coordinate);
            } else if (!_path.isEmpty()) {
                // Colinear — replace the end vertex to keep path simplified
                _path.replaceCoordinate(_path.size() - 1, coordinate);
            }
            _lastPoint = coordinate;
            emit pathChanged(_path);
        }
    } else {
        // First point in the trajectory
        _lastPoint = coordinate;
        _path.addCoordinate(coordinate);
        emit pathChanged(_path);
    }
}
