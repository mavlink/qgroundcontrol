/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CoordinateVector.h"

CoordinateVector::CoordinateVector(QObject* parent)
    : QObject(parent)
{

}

CoordinateVector::CoordinateVector(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2, QObject* parent)
    : QObject(parent)
    , _coordinate1(coordinate1)
    , _coordinate2(coordinate2)
{
    
}

void CoordinateVector::setCoordinates(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2)
{
    setCoordinate1(coordinate1);
    setCoordinate2(coordinate2);
}

void CoordinateVector::setCoordinate1(const QGeoCoordinate &coordinate)
{
    if (_coordinate1 != coordinate) {
        _coordinate1 = coordinate;
        emit coordinate1Changed(_coordinate1);
    }
}

void CoordinateVector::setCoordinate2(const QGeoCoordinate &coordinate)
{
    if (_coordinate2 != coordinate) {
        _coordinate2 = coordinate;
        emit coordinate2Changed(_coordinate2);
    }
}

void CoordinateVector::setSpecialVisual(bool specialVisual)
{
    if (_specialVisual != specialVisual) {
        _specialVisual = specialVisual;
        emit specialVisualChanged(specialVisual);
    }
}
