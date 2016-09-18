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
#include "JsonHelper.h"

#include <QGeoRectangle>
#include <QDebug>
#include <QJsonArray>

const char* QGCMapPolygon::_jsonPolygonKey = "polygon";

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

QGeoCoordinate QGCMapPolygon::_coordFromPointF(const QPointF& point) const
{
    QGeoCoordinate coord;

    if (_polygonPath.count() > 0) {
        QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
        convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &coord);
    }

    return coord;
}

QPointF QGCMapPolygon::_pointFFromCoord(const QGeoCoordinate& coordinate) const
{
    if (_polygonPath.count() > 0) {
        double y, x, down;
        QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();

        convertGeoToNed(coordinate, tangentOrigin, &y, &x, &down);
        return QPointF(x, -y);
    }

    return QPointF();
}

QPolygonF QGCMapPolygon::_toPolygonF(void) const
{
    QPolygonF polygon;

    if (_polygonPath.count() > 2) {
        for (int i=0; i<_polygonPath.count(); i++) {
            polygon.append(_pointFFromCoord(_polygonPath[i].value<QGeoCoordinate>()));
        }
    }

    return polygon;
}

bool QGCMapPolygon::containsCoordinate(const QGeoCoordinate& coordinate) const
{
    if (_polygonPath.count() > 2) {
        return _toPolygonF().containsPoint(_pointFFromCoord(coordinate), Qt::OddEvenFill);
    } else {
        return false;
    }
}

QGeoCoordinate QGCMapPolygon::center(void) const
{
    if (_polygonPath.count() > 2) {
        QPointF centerPoint = _toPolygonF().boundingRect().center();
        return _coordFromPointF(centerPoint);
    } else {
        return QGeoCoordinate();
    }
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

void QGCMapPolygon::saveToJson(QJsonObject& json)
{
    QJsonArray rgPoints;

    // Add all points to the array
    for (int i=0; i<_polygonPath.count(); i++) {
        QJsonValue jsonPoint;

        JsonHelper::writeQGeoCoordinate(jsonPoint, (*this)[i], false /* writeAltitude */);
        rgPoints.append(jsonPoint);
    }

    json.insert(_jsonPolygonKey, QJsonValue(rgPoints));

    setDirty(false);
}

bool QGCMapPolygon::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();
    clear();

    if (required) {
        if (!JsonHelper::validateRequiredKeys(json, QStringList(_jsonPolygonKey), errorString)) {
            return false;
        }
    } else if (!json.contains(_jsonPolygonKey)) {
        return true;
    }

    QList<QJsonValue::Type> types;

    types << QJsonValue::Array;
    if (!JsonHelper::validateKeyTypes(json, QStringList(_jsonPolygonKey), types, errorString)) {
        return false;
    }

    QList<QGeoCoordinate> rgPoints;
    QJsonArray jsonPoints =  json[_jsonPolygonKey].toArray();
    for (int i=0; i<jsonPoints.count(); i++) {
        QGeoCoordinate coordinate;

        if (!JsonHelper::toQGeoCoordinate(jsonPoints[i], coordinate, false /* altitudeRequired */, errorString)) {
            return false;
        }
        rgPoints.append(coordinate);
    }
    setPath(rgPoints);

    setDirty(false);

    return true;
}

QList<QGeoCoordinate> QGCMapPolygon::coordinateList(void) const
{
    QList<QGeoCoordinate> coords;

    for (int i=0; i<count(); i++) {
        coords.append((*this)[i]);
    }

    return coords;
}
