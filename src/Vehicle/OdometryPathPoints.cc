/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OdometryPathPoints.h"
#include "Vehicle.h"
#include "QGCGeo.h"

#include <QtCore/QDebug>
#include <cmath>

OdometryPathPoints::OdometryPathPoints(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    qDebug() << "OdometryPathPoints created for vehicle" << vehicle->id();
}

void OdometryPathPoints::setEnabled(bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        qDebug() << "Odometry Path enabled:" << _enabled;
        if (!_enabled) {
            clear();
        } else {
            // Set reference coordinate when enabling
            _referenceCoordinate = _vehicle->homePosition();
            if (!_referenceCoordinate.isValid()) {
                _referenceCoordinate = _vehicle->coordinate();
            }
            qDebug() << "Odometry Path reference coordinate:" << _referenceCoordinate;
        }
        emit enabledChanged();
    }
}

void OdometryPathPoints::addOdometryPoint(double x, double y, double z)
{
    if (!_enabled) {
        return;
    }

    // Update reference if we don't have one yet
    if (!_referenceCoordinate.isValid()) {
        _referenceCoordinate = _vehicle->homePosition();
        if (!_referenceCoordinate.isValid()) {
            _referenceCoordinate = _vehicle->coordinate();
        }
        qDebug() << "Odometry Path updated reference:" << _referenceCoordinate;
    }

    if (!_referenceCoordinate.isValid()) {
        qDebug() << "Odometry Path: No valid reference coordinate";
        return; // Still no valid reference, can't convert
    }

    // Check for NaN values
    if (std::isnan(x) || std::isnan(y)) {
        qDebug() << "Odometry Path: NaN values in data";
        return;
    }

    // Convert NED (odometry x=north, y=east, z=down) to geodetic coordinate
    // Note: z should be positive down in NED convention
    QGeoCoordinate coordinate;
    QGCGeo::convertNedToGeo(x, y, z, _referenceCoordinate, coordinate);

    if (!coordinate.isValid()) {
        qDebug() << "Odometry Path: Invalid converted coordinate from NED:" << x << y << z;
        return;
    }

    _lastPoint = coordinate;
    _points.append(QVariant::fromValue(coordinate));

    if (_points.size() > _maxPointCount) {
        _points.removeFirst();
    }

    qDebug() << "Odometry Path point added:" << coordinate << "from NED:" << x << y << z << "Total:" << _points.size();
    emit pointAdded(coordinate);
}

void OdometryPathPoints::clear(void)
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    emit pointsCleared();
}

