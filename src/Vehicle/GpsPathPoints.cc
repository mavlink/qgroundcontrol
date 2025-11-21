/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GpsPathPoints.h"
#include "Vehicle.h"
#include <QtCore/QDebug>

GpsPathPoints::GpsPathPoints(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    qDebug() << "GpsPathPoints created for vehicle" << vehicle->id();
}

void GpsPathPoints::setEnabled(bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        qDebug() << "GPS Path enabled:" << _enabled;
        if (!_enabled) {
            clear();
        }
        emit enabledChanged();
    }
}

void GpsPathPoints::addGpsRawIntPoint(QGeoCoordinate coordinate)
{
    if (!_enabled) {
        return;
    }
    
    if (!coordinate.isValid()) {
        qDebug() << "GPS Path: Invalid coordinate";
        return;
    }

    _lastPoint = coordinate;
    _points.append(QVariant::fromValue(coordinate));

    if (_points.size() > _maxPointCount) {
        _points.removeFirst();
    }

    qDebug() << "GPS Path point added:" << coordinate << "Total:" << _points.size();
    emit pointAdded(coordinate);
}

void GpsPathPoints::clear(void)
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    emit pointsCleared();
}

