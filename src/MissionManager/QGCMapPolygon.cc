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
#include "QGCApplication.h"

#include <QGeoRectangle>
#include <QDebug>
#include <QJsonArray>
#include <QLineF>
#include <QFile>
#include <QDomDocument>

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
    connect(this, &QGCMapPolygon::pathChanged, this, &QGCMapPolygon::_updateCenter);
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

QGeoCoordinate QGCMapPolygon::vertexCoordinate(int vertex) const
{
    if (vertex >= 0 && vertex < _polygonPath.count()) {
        return _polygonPath[vertex].value<QGeoCoordinate>();
    } else {
        qWarning() << "QGCMapPolygon::vertexCoordinate bad vertex requested";
        return QGeoCoordinate();
    }
}

QList<QPointF> QGCMapPolygon::nedPolygon(void)
{
    QList<QPointF>  nedPolygon;

    if (count() > 0) {
        QGeoCoordinate  tangentOrigin = vertexCoordinate(0);

        for (int i=0; i<_polygonModel.count(); i++) {
            double y, x, down;
            QGeoCoordinate vertex = vertexCoordinate(i);
            if (i == 0) {
                // This avoids a nan calculation that comes out of convertGeoToNed
                x = y = 0;
            } else {
                convertGeoToNed(vertex, tangentOrigin, &y, &x, &down);
            }
            nedPolygon += QPointF(x, y);
        }
    }

    return nedPolygon;
}


void QGCMapPolygon::offset(double distance)
{
    QList<QGeoCoordinate> rgNewPolygon;

    // I'm sure there is some beautiful famous algorithm to do this, but here is a brute force method

    if (count() > 2) {
        // Convert the polygon to NED
        QList<QPointF> rgNedVertices = nedPolygon();

        // Walk the edges, offsetting by the specified distance
        QList<QLineF> rgOffsetEdges;
        for (int i=0; i<rgNedVertices.count(); i++) {
            int     lastIndex = i == rgNedVertices.count() - 1 ? 0 : i + 1;
            QLineF  offsetEdge;
            QLineF  originalEdge(rgNedVertices[i], rgNedVertices[lastIndex]);

            QLineF workerLine = originalEdge;
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() - 90.0);
            offsetEdge.setP1(workerLine.p2());

            workerLine.setPoints(originalEdge.p2(), originalEdge.p1());
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() + 90.0);
            offsetEdge.setP2(workerLine.p2());

            rgOffsetEdges.append(offsetEdge);
        }

        // Intersect the offset edges to generate new vertices
        QPointF         newVertex;
        QGeoCoordinate  tangentOrigin = vertexCoordinate(0);
        for (int i=0; i<rgOffsetEdges.count(); i++) {
            int prevIndex = i == 0 ? rgOffsetEdges.count() - 1 : i - 1;
            if (rgOffsetEdges[prevIndex].intersect(rgOffsetEdges[i], &newVertex) == QLineF::NoIntersection) {
                // FIXME: Better error handling?
                qWarning("Intersection failed");
                return;
            }
            QGeoCoordinate coord;
            convertNedToGeo(newVertex.y(), newVertex.x(), 0, tangentOrigin, &coord);
            rgNewPolygon.append(coord);
        }
    }

    // Update internals
    clear();
    for (int i=0; i<rgNewPolygon.count(); i++) {
        appendVertex(rgNewPolygon[i]);
    }
}

bool QGCMapPolygon::loadKMLFile(const QString& kmlFile)
{
    QFile file(kmlFile);

    if (!file.exists()) {
        qgcApp()->showMessage(tr("File not found: %1").arg(kmlFile));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qgcApp()->showMessage(tr("Unable to open file: %1 error: $%2").arg(kmlFile).arg(file.errorString()));
        return false;
    }

    QDomDocument doc;
    QString errorMessage;
    int errorLine;
    if (!doc.setContent(&file, &errorMessage, &errorLine)) {
        qgcApp()->showMessage(tr("Unable to parse KML file: %1 error: %2 line: %3").arg(kmlFile).arg(errorMessage).arg(errorLine));
        return false;
    }

    QDomNodeList rgNodes = doc.elementsByTagName("Polygon");
    if (rgNodes.count() == 0) {
        qgcApp()->showMessage(tr("Unable to find Polygon node in KML"));
        return false;
    }

    QDomNode coordinatesNode = rgNodes.item(0).namedItem("outerBoundaryIs").namedItem("LinearRing").namedItem("coordinates");
    if (coordinatesNode.isNull()) {
        qgcApp()->showMessage(tr("Internal error: Unable to find coordinates node in KML"));
        return false;
    }

    QString coordinatesString = coordinatesNode.toElement().text().simplified();
    QStringList rgCoordinateStrings = coordinatesString.split(" ");

    QList<QGeoCoordinate> rgCoords;
    for (int i=0; i<rgCoordinateStrings.count()-1; i++) {
        QString coordinateString = rgCoordinateStrings[i];

        QStringList rgValueStrings = coordinateString.split(",");

        QGeoCoordinate coord;
        coord.setLongitude(rgValueStrings[0].toDouble());
        coord.setLatitude(rgValueStrings[1].toDouble());

        rgCoords.append(coord);
    }

    // Determine winding, reverse if needed
    double sum = 0;
    for (int i=0; i<rgCoords.count(); i++) {
        QGeoCoordinate coord1 = rgCoords[i];
        QGeoCoordinate coord2 = (i == rgCoords.count() - 1) ? rgCoords[0] : rgCoords[i+1];

        sum += (coord2.longitude() - coord1.longitude()) * (coord2.latitude() + coord1.latitude());
    }
    bool reverse = sum < 0.0;

    if (reverse) {
        QList<QGeoCoordinate> rgReversed;

        for (int i=0; i<rgCoords.count(); i++) {
            rgReversed.prepend(rgCoords[i]);
        }
        rgCoords = rgReversed;
    }

    clear();
    for (int i=0; i<rgCoords.count(); i++) {
        appendVertex(rgCoords[i]);
    }

    return true;
}
