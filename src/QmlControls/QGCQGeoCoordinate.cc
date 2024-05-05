/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCQGeoCoordinate.h"

#include <QtQml/QQmlEngine>

QGCQGeoCoordinate::QGCQGeoCoordinate(const QGeoCoordinate& coord, QObject* parent)
    : QmlObjectListItem(parent)
    , _coordinate   (coord)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

void QGCQGeoCoordinate::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(coordinate);
        setDirty(true);
    }
}
