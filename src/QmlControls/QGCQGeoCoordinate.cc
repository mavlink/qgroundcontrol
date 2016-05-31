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

#include "QGCQGeoCoordinate.h"

QGCQGeoCoordinate::QGCQGeoCoordinate(QObject* parent)
    : QObject(parent)
{

}

QGCQGeoCoordinate::QGCQGeoCoordinate(const QGeoCoordinate& coordinate, QObject* parent)
    : QObject(parent)
    , _coordinate(coordinate)
{
    
}

QGCQGeoCoordinate::~QGCQGeoCoordinate()
{
    
}

void QGCQGeoCoordinate::setCoordinate(const QGeoCoordinate& coordinate)
{
    _coordinate = coordinate;
    emit coordinateChanged(_coordinate);
}
