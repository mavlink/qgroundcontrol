#include "TrajectoryPoints.h"
#include "Vehicle.h"

#include <QtCore/qmath.h>

#include <cmath>

TrajectoryPoints::TrajectoryPoints(Vehicle* vehicle, QObject* parent)
    : QObject   (parent)
    , _vehicle  (vehicle)
{
}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate)
{
    // The goal of this algorithm is to limit the number of trajectory points whic represent the vehicle path.
    // Fewer points means higher performance of map display.

    if (_lastPoint.isValid()) {
        const double horizontalDistance = _lastPoint.distanceTo(coordinate);
        double altitudeDelta = coordinate.altitude() - _lastPoint.altitude();
        if (!std::isfinite(altitudeDelta)) {
            altitudeDelta = 0.0;
        }
        // Use 3D distance so purely vertical climbs/descents also generate trajectory points
        const double distance = std::hypot(horizontalDistance, altitudeDelta);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
            _vehicle->updateFlightDistance(distance);
            // Vehicle has moved far enough from previous point for an update.
            // Unit direction of the new segment in local east/north/up coordinates.
            double east = 0.0;
            double north = 0.0;
            if (horizontalDistance > 0.0) {
                const double azimuthRad = qDegreesToRadians(_lastPoint.azimuthTo(coordinate));
                east = std::sin(azimuthRad) * horizontalDistance / distance;
                north = std::cos(azimuthRad) * horizontalDistance / distance;
            }
            const QVector3D newDirection(static_cast<float>(east), static_cast<float>(north),
                                         static_cast<float>(altitudeDelta / distance));
            static const float colinearDotThreshold =
                static_cast<float>(std::cos(qDegreesToRadians(_directionTolerance)));
            if (_lastDirection.isNull() || QVector3D::dotProduct(newDirection, _lastDirection) < colinearDotThreshold) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                _lastDirection = newDirection;
                _lastPoint = coordinate;
                _points.append(QVariant::fromValue(coordinate));
                emit pointAdded(coordinate);
            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                _lastPoint = coordinate;
                _points[_points.count() - 1] = QVariant::fromValue(coordinate);
                emit updateLastPoint(coordinate);
            }
        }
    } else {
        // Add the very first trajectory point to the list
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
    disconnect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::clear(void)
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    _lastDirection = QVector3D();
    emit pointsCleared();
}
