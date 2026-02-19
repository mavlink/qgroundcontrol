#include "QGCMapPolygon.h"

#include <QtCore/QLineF>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QTimer>

#include "GeoFormatRegistry.h"
#include "GeoUtilities.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "QGCQGeoCoordinate.h"

QGC_LOGGING_CATEGORY(QGCMapPolygonLog, "QmlControls.QGCMapPolygon")

QGCMapPolygon::QGCMapPolygon(QObject* parent)
    : QGCMapPathBase(parent)
{
}

QGCMapPolygon::QGCMapPolygon(const QGCMapPolygon& other, QObject* parent)
    : QGCMapPathBase(parent)
{
    *this = other;
}

const QGCMapPolygon& QGCMapPolygon::operator=(const QGCMapPolygon& other)
{
    QGCMapPathBase::operator=(other);
    return *this;
}

void QGCMapPolygon::_clearImpl()
{
    // Bug workaround, see below
    while (_path.count() > 1) {
        _path.takeLast();
    }
    emit pathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in QGCMapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _path.clear();
}

void QGCMapPolygon::adjustVertex(int vertexIndex, const QGeoCoordinate coordinate)
{
    if (vertexIndex < 0 || vertexIndex >= _path.count()) {
        qCWarning(QGCMapPolygonLog) << "adjustVertex called with invalid index:" << vertexIndex << "count:" << _path.count();
        return;
    }

    _path[vertexIndex] = QVariant::fromValue(coordinate);
    _model.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);
    if (!_centerDrag && !_deferredPathChanged) {
        // When dragging center we don't signal path changed until all vertices are updated
        _deferredPathChanged = true;
        QTimer::singleShot(0, this, [this]() {
            emit pathChanged();
            _deferredPathChanged = false;
        });
    }
    setDirty(true);
}

void QGCMapPolygon::splitPolygonSegment(int vertexIndex)
{
    splitSegment(vertexIndex);
}

void QGCMapPolygon::_onPathChanged()
{
    _updateCenter();
}

void QGCMapPolygon::_onModelReset()
{
    QGCMapPathBase::_onModelReset();
    emit centerChanged(_center);
}

bool QGCMapPolygon::_loadFromFile(const QString& file, QList<QList<QGeoCoordinate>>& coords, QString& errorString)
{
    return GeoFormatRegistry::loadPolygons(file, coords, errorString);
}

void QGCMapPolygon::appendVertices(const QVariantList& varCoords)
{
    QList<QGeoCoordinate> rgCoords;
    for (const QVariant& varCoord : varCoords) {
        rgCoords.append(varCoord.value<QGeoCoordinate>());
    }
    QGCMapPathBase::appendVertices(rgCoords);
}

QPolygonF QGCMapPolygon::_toPolygonF() const
{
    QPolygonF polygon;

    if (_path.count() > 2) {
        for (int i = 0; i < _path.count(); i++) {
            polygon.append(_pointFFromCoord(_path[i].value<QGeoCoordinate>()));
        }
    }

    return polygon;
}

bool QGCMapPolygon::containsCoordinate(const QGeoCoordinate& coordinate) const
{
    if (_path.count() > 2) {
        return _toPolygonF().containsPoint(_pointFFromCoord(coordinate), Qt::OddEvenFill);
    } else {
        return false;
    }
}

void QGCMapPolygon::_scheduleDeferredCenterChanged(const QGeoCoordinate& center)
{
    _pendingCenter = center;
    if (_deferredCenterChanged) {
        return;
    }

    _deferredCenterChanged = true;
    QTimer::singleShot(0, this, [this]() {
        emit centerChanged(_pendingCenter);
        _deferredCenterChanged = false;
    });
}

void QGCMapPolygon::_updateCenter()
{
    if (!_ignoreCenterUpdates) {
        if (_path.count() <= 2) {
            if (_center.isValid()) {
                _center = QGeoCoordinate();
                emit centerChanged(_center);
            }
            return;
        }

        const QGeoCoordinate center = GeoUtilities::polygonCentroid(coordinateList());

        if (_center != center) {
            _center = center;
            emit centerChanged(center);
        }
    }
}

void QGCMapPolygon::setCenter(QGeoCoordinate newCenter)
{
    if (newCenter != _center) {
        const QScopedValueRollback<bool> ignoreCenterUpdates(_ignoreCenterUpdates, true);

        const double distance = _center.distanceTo(newCenter);
        const double azimuth = _center.azimuthTo(newCenter);

        const qsizetype pointCount = count();
        for (qsizetype i = 0; i < pointCount; ++i) {
            const QGeoCoordinate oldVertex = _path[i].value<QGeoCoordinate>();
            const QGeoCoordinate newVertex = oldVertex.atDistanceAndAzimuth(distance, azimuth);
            adjustVertex(static_cast<int>(i), newVertex);
        }

        if (_centerDrag && !_deferredPathChanged) {
            _deferredPathChanged = true;
            QTimer::singleShot(0, this, [this]() {
                emit pathChanged();
                _deferredPathChanged = false;
            });
        }

        _center = newCenter;
        _scheduleDeferredCenterChanged(newCenter);
    }
}

void QGCMapPolygon::setCenterDrag(bool centerDrag)
{
    if (centerDrag != _centerDrag) {
        _centerDrag = centerDrag;
        emit centerDragChanged(centerDrag);
    }
}

void QGCMapPolygon::offset(double distance)
{
    QList<QGeoCoordinate> rgNewPolygon;

    if (count() > 2) {
        const QList<QPointF> rgNedVertices = nedPath();

        QList<QLineF> rgOffsetEdges;
        const qsizetype vertexCount = rgNedVertices.count();
        for (qsizetype i = 0; i < vertexCount; ++i) {
            const qsizetype lastIndex = i == vertexCount - 1 ? 0 : i + 1;
            QLineF offsetEdge;
            const QLineF originalEdge(rgNedVertices[i], rgNedVertices[lastIndex]);

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

        QPointF newVertex;
        const QGeoCoordinate tangentOrigin = vertexCoordinate(0);
        const qsizetype edgeCount = rgOffsetEdges.count();
        for (qsizetype i = 0; i < edgeCount; ++i) {
            const qsizetype prevIndex = i == 0 ? edgeCount - 1 : i - 1;
            const auto intersect = rgOffsetEdges[prevIndex].intersects(rgOffsetEdges[i], &newVertex);
            if (intersect == QLineF::NoIntersection) {
                qCWarning(QGCMapPolygonLog) << "Offset edge intersection failed at edge index" << i;
                return;
            }
            QGeoCoordinate coord;
            QGCGeo::convertNedToGeo(newVertex.y(), newVertex.x(), 0, tangentOrigin, coord);
            rgNewPolygon.append(coord);
        }
    }

    clear();
    appendVertices(rgNewPolygon);
}

double QGCMapPolygon::area() const
{
    return GeoUtilities::polygonArea(coordinateList());
}

void QGCMapPolygon::verifyClockwiseWinding()
{
    if (_path.count() <= 2) {
        return;
    }

    QList<QGeoCoordinate> coords = coordinateList();
    if (GeoUtilities::ensureClockwise(coords)) {
        clear();
        appendVertices(coords);
    }
}

void QGCMapPolygon::setShowAltColor(bool showAltColor)
{
    if (showAltColor != _showAltColor) {
        _showAltColor = showAltColor;
        emit showAltColorChanged(showAltColor);
    }
}
