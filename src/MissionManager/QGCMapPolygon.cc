/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapPolygon.h"
#include "QGCGeo.h"

#include <QGeoRectangle>
#include <QDebug>
#include <QPolygon>

QGCMapPolygon::QGCMapPolygon(QObject* parent)
    : QObject(parent)
    , _dirty(false)
{

}

const QGCMapPolygon& QGCMapPolygon::operator=(const QGCMapPolygon& other)
{
    _polygonPath = other._polygonPath;
    setDirty(true);

    emit pathChanged();

    return *this;
}

void QGCMapPolygon::clear(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit pathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in QGCMapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();

    setDirty(true);
}

void QGCMapPolygon::addCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit pathChanged();
    setDirty(true);
}

void QGCMapPolygon::adjustCoordinate(int vertexIndex, const QGeoCoordinate coordinate)
{
    _polygonPath[vertexIndex] = QVariant::fromValue(coordinate);
    emit pathChanged();
    setDirty(true);
}

void QGCMapPolygon::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

QGeoCoordinate QGCMapPolygon::center(void) const
{
    QPolygonF polygon;

    QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();

    foreach(const QVariant& coordVar, _polygonPath) {
        double y, x, down;

        convertGeoToNed(coordVar.value<QGeoCoordinate>(), tangentOrigin, &y, &x, &down);
        polygon << QPointF(x, -y);
    }

    QGeoCoordinate centerCoord;
    QPointF centerPoint = polygon.boundingRect().center();
    convertNedToGeo(-centerPoint.y(), centerPoint.x(), 0, tangentOrigin, &centerCoord);

    return centerCoord;
}

void QGCMapPolygon::setPath(const QList<QGeoCoordinate>& path)
{
    _polygonPath.clear();
    foreach(const QGeoCoordinate& coord, path) {
        _polygonPath << QVariant::fromValue(coord);
    }
    setDirty(true);
    emit pathChanged();
}

void QGCMapPolygon::setPath(const QVariantList& path)
{
    _polygonPath = path;
    setDirty(true);
    emit pathChanged();
}
