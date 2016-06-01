/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

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

CoordinateVector::~CoordinateVector()
{
    
}

void CoordinateVector::setCoordinates(const QGeoCoordinate& coordinate1, const QGeoCoordinate& coordinate2)
{
    _coordinate1 = coordinate1;
    _coordinate2 = coordinate2;
    emit coordinate1Changed(_coordinate1);
    emit coordinate2Changed(_coordinate2);
}
