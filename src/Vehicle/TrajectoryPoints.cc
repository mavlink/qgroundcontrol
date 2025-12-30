/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
{

}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate,uint8_t src)
{
    // The goal of this algorithm is to limit the number of trajectory points whic represent the vehicle path.
    // Fewer points means higher performance of map display.
    qDebug()<<"handle "<<src<<"trajectory";
    auto &trajectory =_trajectories[src];
    if ( trajectory.lastPoint.isValid()) {
        double distance =  trajectory.lastPoint.distanceTo(coordinate);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
            _vehicle->updateFlightDistance(distance);
            // Vehicle has moved far enough from previous point for an update
            double newAzimuth =  trajectory.lastPoint.azimuthTo(coordinate);
            if (qIsNaN(trajectory.lastAzimuth) || qAbs(newAzimuth - trajectory.lastAzimuth) > _azimuthTolerance) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                trajectory.lastAzimuth = trajectory.lastPoint.azimuthTo(coordinate);
                trajectory.lastPoint = coordinate;
                trajectory.points.append(QVariant::fromValue(coordinate));
                emit pointAdded(coordinate, src);
            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                trajectory.lastPoint = coordinate;
                trajectory.points.last() = QVariant::fromValue(coordinate);
                emit updateLastPoint(coordinate, src);
            }
        }
    } else {
        // Add the very first trajectory point to the list
        trajectory.lastPoint = coordinate;
        trajectory.points.append(QVariant::fromValue(coordinate));
        emit pointAdded(coordinate, src);
    }
}

void TrajectoryPoints::start(void)
{
    clear();
    connect(_vehicle, &Vehicle::updateTrajectory, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::stop(void)
{
    disconnect(_vehicle, &Vehicle::updateTrajectory, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::clear(void)
{
    _trajectories.clear();
    emit pointsCleared();
}
