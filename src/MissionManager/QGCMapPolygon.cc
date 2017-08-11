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
#include "QGCQGeoCoordinate.h"

#include <QGeoRectangle>
#include <QDebug>
#include <QJsonArray>

const char* QGCMapPolygon::jsonPolygonKey = "polygon";

QGCMapPolygon::QGCMapPolygon(QObject* parent)
    : QObject               (parent)
    , _dirty                (false)
    , _centerDrag           (false)
    , _ignoreCenterUpdates  (false)
    , _interactive          (false)
{
    _init();
}

QGCMapPolygon::QGCMapPolygon(const QGCMapPolygon& other, QObject* parent)
    : QObject               (parent)
    , _dirty                (false)
    , _centerDrag           (false)
    , _ignoreCenterUpdates  (false)
    , _interactive          (false)
{
    *this = other;

    _init();
}

void QGCMapPolygon::_init(void)
{
    connect(&_polygonModel, &QmlObjectListModel::dirtyChanged, this, &QGCMapPolygon::_polygonModelDirtyChanged);
    connect(&_polygonModel, &QmlObjectListModel::countChanged, this, &QGCMapPolygon::_polygonModelCountChanged);
    connect(&_polygonModel, &QmlObjectListModel::countChanged, this, &QGCMapPolygon::_updateCenter);
}

const QGCMapPolygon& QGCMapPolygon::operator=(const QGCMapPolygon& other)
{
    clear();

    QVariantList vertices = other.path();
    for (int i=0; i<vertices.count(); i++) {
        appendVertex(vertices[i].value<QGeoCoordinate>());
    }

    setDirty(true);

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

    _polygonModel.clearAndDeleteContents();

    emit cleared();

    setDirty(true);
}

void QGCMapPolygon::adjustVertex(int vertexIndex, const QGeoCoordinate coordinate)
{
    _polygonPath[vertexIndex] = QVariant::fromValue(coordinate);
    if (!_centerDrag) {
        // When dragging center we don't signal path changed until add vertices are updated
        emit pathChanged();
    }

    _polygonModel.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);

    if (!_ignoreCenterUpdates) {
        _updateCenter();
    }

    setDirty(true);
}

void QGCMapPolygon::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            _polygonModel.setDirty(false);
        }
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

void QGCMapPolygon::setPath(const QList<QGeoCoordinate>& path)
{
    _polygonPath.clear();
    _polygonModel.clearAndDeleteContents();
    foreach(const QGeoCoordinate& coord, path) {
        _polygonPath.append(QVariant::fromValue(coord));
        _polygonModel.append(new QGCQGeoCoordinate(coord, this));
    }

    setDirty(true);
    emit pathChanged();
}

void QGCMapPolygon::setPath(const QVariantList& path)
{
    _polygonPath = path;

    _polygonModel.clearAndDeleteContents();
    for (int i=0; i<_polygonPath.count(); i++) {
        _polygonModel.append(new QGCQGeoCoordinate(_polygonPath[i].value<QGeoCoordinate>(), this));
    }

    setDirty(true);
    emit pathChanged();
}

void QGCMapPolygon::saveToJson(QJsonObject& json)
{
    QJsonValue jsonValue;

    JsonHelper::saveGeoCoordinateArray(_polygonPath, false /* writeAltitude*/, jsonValue);
    json.insert(jsonPolygonKey, jsonValue);
    setDirty(false);
}

bool QGCMapPolygon::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();
    clear();

    if (required) {
        if (!JsonHelper::validateRequiredKeys(json, QStringList(jsonPolygonKey), errorString)) {
            return false;
        }
    } else if (!json.contains(jsonPolygonKey)) {
        return true;
    }

    if (!JsonHelper::loadGeoCoordinateArray(json[jsonPolygonKey], false /* altitudeRequired */, _polygonPath, errorString)) {
        return false;
    }

    for (int i=0; i<_polygonPath.count(); i++) {
        _polygonModel.append(new QGCQGeoCoordinate(_polygonPath[i].value<QGeoCoordinate>(), this));
    }

    setDirty(false);
    emit pathChanged();

    return true;
}

QList<QGeoCoordinate> QGCMapPolygon::coordinateList(void) const
{
    QList<QGeoCoordinate> coords;

    for (int i=0; i<_polygonPath.count(); i++) {
        coords.append(_polygonPath[i].value<QGeoCoordinate>());
    }

    return coords;
}

void QGCMapPolygon::splitPolygonSegment(int vertexIndex)
{
    int nextIndex = vertexIndex + 1;
    if (nextIndex > _polygonPath.length() - 1) {
        nextIndex = 0;
    }

    QGeoCoordinate firstVertex = _polygonPath[vertexIndex].value<QGeoCoordinate>();
    QGeoCoordinate nextVertex = _polygonPath[nextIndex].value<QGeoCoordinate>();

    double distance = firstVertex.distanceTo(nextVertex);
    double azimuth = firstVertex.azimuthTo(nextVertex);
    QGeoCoordinate newVertex = firstVertex.atDistanceAndAzimuth(distance / 2, azimuth);

    if (nextIndex == 0) {
        appendVertex(newVertex);
    } else {
        _polygonModel.insert(nextIndex, new QGCQGeoCoordinate(newVertex, this));
        _polygonPath.insert(nextIndex, QVariant::fromValue(newVertex));
        emit pathChanged();
    }
}

void QGCMapPolygon::appendVertex(const QGeoCoordinate& coordinate)
{
    _polygonPath.append(QVariant::fromValue(coordinate));
    _polygonModel.append(new QGCQGeoCoordinate(coordinate, this));
    emit pathChanged();
}

void QGCMapPolygon::_polygonModelDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void QGCMapPolygon::removeVertex(int vertexIndex)
{
    if (vertexIndex < 0 && vertexIndex > _polygonPath.length() - 1) {
        qWarning() << "Call to removePolygonCoordinate with bad vertexIndex:count" << vertexIndex << _polygonPath.length();
        return;
    }

    if (_polygonPath.length() <= 3) {
        // Don't allow the user to trash the polygon
        return;
    }

    QObject* coordObj = _polygonModel.removeAt(vertexIndex);
    coordObj->deleteLater();

    _polygonPath.removeAt(vertexIndex);
    emit pathChanged();

    _updateCenter();
}

void QGCMapPolygon::_polygonModelCountChanged(int count)
{
    emit countChanged(count);
}

void QGCMapPolygon::_updateCenter(void)
{
    if (!_ignoreCenterUpdates) {
        QGeoCoordinate center;

        if (_polygonPath.count() > 2) {            
            QPointF centroid(0, 0);
            QPolygonF polygonF = _toPolygonF();
            for (int i=0; i<polygonF.count(); i++) {
                centroid += polygonF[i];
            }
            center = _coordFromPointF(QPointF(centroid.x() / polygonF.count(), centroid.y() / polygonF.count()));
        }

        _center = center;
        emit centerChanged(center);
    }
}

void QGCMapPolygon::setCenter(QGeoCoordinate newCenter)
{
    if (newCenter != _center) {
        _ignoreCenterUpdates = true;

        // Adjust polygon vertices to new center
        double distance = _center.distanceTo(newCenter);
        double azimuth = _center.azimuthTo(newCenter);

        for (int i=0; i<count(); i++) {
            QGeoCoordinate oldVertex = _polygonPath[i].value<QGeoCoordinate>();
            QGeoCoordinate newVertex = oldVertex.atDistanceAndAzimuth(distance, azimuth);
            adjustVertex(i, newVertex);
        }

        if (_centerDrag) {
            // When center dragging signals are delayed until all vertices are updated
            emit pathChanged();
        }

        _ignoreCenterUpdates = false;

        _center = newCenter;
        emit centerChanged(newCenter);
    }
}

void QGCMapPolygon::setCenterDrag(bool centerDrag)
{
    if (centerDrag != _centerDrag) {
        _centerDrag = centerDrag;
        emit centerDragChanged(centerDrag);
    }
}

void QGCMapPolygon::setInteractive(bool interactive)
{
    if (_interactive != interactive) {
        _interactive = interactive;
        emit interactiveChanged(interactive);
    }
}
