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
    , _lastAzimuth  (qQNaN())
{
}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate, PositionSrc src)
{
    // The goal of this algorithm is to limit the number of trajectory points whic represent the vehicle path.
    // Fewer points means higher performance of map display.
    QGeoCoordinate &lastPoint = (src == PositionSrc::eSrc_GlobalPosition) ? _lastPoint   : _gpsLastPoit   ;
    QVariantList   &pointList = (src == PositionSrc::eSrc_GlobalPosition) ? _points      : _gpsPoints     ;
    double         &athimuth  = (src == PositionSrc::eSrc_GlobalPosition) ? _lastAzimuth : _gpsLastAzimuth;

    if (lastPoint.isValid()) {
        double distance = lastPoint.distanceTo(coordinate);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
            _vehicle->updateFlightDistance(distance);
            // Vehicle has moved far enough from previous point for an update
            double newAzimuth = lastPoint.azimuthTo(coordinate);
            if (qIsNaN(athimuth) || qAbs(newAzimuth - athimuth) > _azimuthTolerance) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                athimuth = lastPoint.azimuthTo(coordinate);
                lastPoint = coordinate;
                pointList.append(QVariant::fromValue(coordinate));
                if (src == PositionSrc::eSrc_GlobalPosition){
                    emit pointAdded(coordinate);
                } else {
                    emit gpsPointAdded(coordinate);
                }
            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                lastPoint = coordinate;
                pointList[pointList.count() - 1] = QVariant::fromValue(coordinate);
                if (src == PositionSrc::eSrc_GlobalPosition){
                    emit updateLastPoint(coordinate);
                } else {
                    emit gpsUpdateLastPoint(coordinate);
                }
            }
        }
    } else {
        // Add the very first trajectory point to the list
        lastPoint = coordinate;
        pointList.append(QVariant::fromValue(coordinate));
        if (src == PositionSrc::eSrc_GlobalPosition){
            emit pointAdded(coordinate);
        } else {
            emit gpsPointAdded(coordinate);
        }
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
    _points.clear();
    _gpsPoints.clear();
    _lastPoint = QGeoCoordinate();
    _gpsLastPoit = QGeoCoordinate();
    _gpsLastAzimuth = qQNaN();
    _lastAzimuth = qQNaN();
    emit pointsCleared();
}
