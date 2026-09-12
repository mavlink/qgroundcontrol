#include "QGCMapPolyline.h"

#include <QMetaMethod>

#include "AppMessages.h"
#include "GeoJsonHelper.h"
#include "JsonParsing.h"
#include "QGCApplication.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "QGCPolygonClipper.h"
#include "QGCQGeoCoordinate.h"
#include "ShapeFileHelper.h"

QGC_LOGGING_CATEGORY(QGCMapPolylineLog, "QMLControls.QGCMapPolyline")

QGCMapPolyline::QGCMapPolyline(QObject* parent) : QObject(parent), _dirty(false), _interactive(false)
{
    _init();
}

QGCMapPolyline::QGCMapPolyline(const QGCMapPolyline& other, QObject* parent)
    : QObject               (parent)
    , _dirty                (false)
    , _interactive          (false)
{
    *this = other;

    _init();
}

QGCMapPolyline::~QGCMapPolyline()
{
    qgcApp()->removeCompressedSignal(QMetaMethod::fromSignal(&QGCMapPolyline::pathChanged));
}

const QGCMapPolyline& QGCMapPolyline::operator=(const QGCMapPolyline& other)
{
    clear();

    QVariantList vertices = other.path();
    for (int i=0; i<vertices.count(); i++) {
        appendVertex(vertices[i].value<QGeoCoordinate>());
    }

    setDirty(true);

    return *this;
}

void QGCMapPolyline::_init(void)
{
    connect(&_polylineModel, &QmlObjectListModel::dirtyChanged, this, &QGCMapPolyline::_polylineModelDirtyChanged);
    connect(&_polylineModel, &QmlObjectListModel::countChanged, this, &QGCMapPolyline::_polylineModelCountChanged);
    connect(&_polylineModel, &QmlObjectListModel::modelReset, this, &QGCMapPolyline::pathChanged);

    connect(this, &QGCMapPolyline::countChanged, this, &QGCMapPolyline::isValidChanged);
    connect(this, &QGCMapPolyline::countChanged, this, &QGCMapPolyline::isEmptyChanged);

    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&QGCMapPolyline::pathChanged));
}

void QGCMapPolyline::clear(void)
{
    _polylinePath.clear();
    emit pathChanged();

    _polylineModel.clearAndDeleteContents();

    emit cleared();

    setDirty(true);
}

void QGCMapPolyline::adjustVertex(int vertexIndex, const QGeoCoordinate coordinate)
{
    _polylinePath[vertexIndex] = QVariant::fromValue(coordinate);
    _polylineModel.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);
    if (!_deferredPathChanged) {
        _deferredPathChanged = true;
        if (_vertexDrag) {
            // During vertex drag only emit the lightweight visual signal
            QTimer::singleShot(0, this, [this]() {
                emit dragPathChanged();
                _deferredPathChanged = false;
            });
        } else {
            QTimer::singleShot(0, this, [this]() {
                emit pathChanged();
                _deferredPathChanged = false;
            });
        }
    }
    setDirty(true);
}

void QGCMapPolyline::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            _polylineModel.setDirty(false);
        }
        emit dirtyChanged(dirty);
    }
}
QGeoCoordinate QGCMapPolyline::_coordFromPointF(const QPointF& point) const
{
    QGeoCoordinate coord;

    if (_polylinePath.count() > 0) {
        QGeoCoordinate tangentOrigin = _polylinePath[0].value<QGeoCoordinate>();
        QGCGeo::convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, coord);
    }

    return coord;
}

QPointF QGCMapPolyline::_pointFFromCoord(const QGeoCoordinate& coordinate) const
{
    if (_polylinePath.count() > 0) {
        double y, x, down;
        QGeoCoordinate tangentOrigin = _polylinePath[0].value<QGeoCoordinate>();

        QGCGeo::convertGeoToNed(coordinate, tangentOrigin, y, x, down);
        return QPointF(x, -y);
    }

    return QPointF();
}

void QGCMapPolyline::setPath(const QList<QGeoCoordinate>& path)
{
    beginReset();

    _polylinePath.clear();
    _polylineModel.clearAndDeleteContents();
    for (const QGeoCoordinate& coord: path) {
        _polylinePath.append(QVariant::fromValue(coord));
        _polylineModel.append(new QGCQGeoCoordinate(coord, this));
    }

    setDirty(true);

    endReset();
}

void QGCMapPolyline::setPath(const QVariantList& path)
{
    beginReset();

    _polylinePath = path;
    _polylineModel.clearAndDeleteContents();
    for (int i=0; i<_polylinePath.count(); i++) {
        _polylineModel.append(new QGCQGeoCoordinate(_polylinePath[i].value<QGeoCoordinate>(), this));
    }
    setDirty(true);

    endReset();
}


void QGCMapPolyline::saveToJson(QJsonObject& json)
{
    QJsonValue jsonValue;

    GeoJsonHelper::saveGeoCoordinateArray(_polylinePath, false /* writeAltitude*/, jsonValue);
    json.insert(jsonPolylineKey, jsonValue);
    setDirty(false);
}

bool QGCMapPolyline::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();
    clear();

    if (required) {
        if (!JsonParsing::validateRequiredKeys(json, QStringList(jsonPolylineKey), errorString)) {
            return false;
        }
    } else if (!json.contains(jsonPolylineKey)) {
        return true;
    }

    if (!GeoJsonHelper::loadGeoCoordinateArray(json[jsonPolylineKey], false /* altitudeRequired */, _polylinePath, errorString)) {
        return false;
    }

    for (int i=0; i<_polylinePath.count(); i++) {
        _polylineModel.append(new QGCQGeoCoordinate(_polylinePath[i].value<QGeoCoordinate>(), this));
    }

    setDirty(false);
    emit pathChanged();

    return true;
}

QList<QGeoCoordinate> QGCMapPolyline::coordinateList(void) const
{
    QList<QGeoCoordinate> coords;

    for (int i=0; i<_polylinePath.count(); i++) {
        coords.append(_polylinePath[i].value<QGeoCoordinate>());
    }

    return coords;
}

void QGCMapPolyline::splitSegment(int vertexIndex)
{
    int nextIndex = vertexIndex + 1;
    if (nextIndex > _polylinePath.length() - 1) {
        return;
    }

    QGeoCoordinate firstVertex = _polylinePath[vertexIndex].value<QGeoCoordinate>();
    QGeoCoordinate nextVertex = _polylinePath[nextIndex].value<QGeoCoordinate>();

    double distance = firstVertex.distanceTo(nextVertex);
    double azimuth = firstVertex.azimuthTo(nextVertex);
    QGeoCoordinate newVertex = firstVertex.atDistanceAndAzimuth(distance / 2, azimuth);

    if (nextIndex == 0) {
        appendVertex(newVertex);
    } else {
        _polylineModel.insert(nextIndex, new QGCQGeoCoordinate(newVertex, this));
        _polylinePath.insert(nextIndex, QVariant::fromValue(newVertex));
        emit pathChanged();
    }
}

void QGCMapPolyline::appendVertex(const QGeoCoordinate& coordinate)
{
    _polylinePath.append(QVariant::fromValue(coordinate));
    _polylineModel.append(new QGCQGeoCoordinate(coordinate, this));
    emit pathChanged();
}

void QGCMapPolyline::removeVertex(int vertexIndex)
{
    if (vertexIndex < 0 || vertexIndex > _polylinePath.length() - 1) {
        qCWarning(QGCMapPolylineLog) << "Call to removeVertex with bad vertexIndex:count" << vertexIndex << _polylinePath.length();
        return;
    }

    if (_polylinePath.length() <= 2) {
        // Don't allow the user to trash the polyline
        return;
    }

    QObject* coordObj = _polylineModel.removeAt(vertexIndex);
    coordObj->deleteLater();
    if(vertexIndex == _selectedVertexIndex) {
        selectVertex(-1);
    } else if (vertexIndex < _selectedVertexIndex) {
        selectVertex(_selectedVertexIndex - 1);
    } // else do nothing - keep current selected vertex

    _polylinePath.removeAt(vertexIndex);
    emit pathChanged();
}

void QGCMapPolyline::setInteractive(bool interactive)
{
    if (_interactive != interactive) {
        _interactive = interactive;
        emit interactiveChanged(interactive);
    }
}

void QGCMapPolyline::setVertexDrag(bool vertexDrag)
{
    if (_vertexDrag != vertexDrag) {
        _vertexDrag = vertexDrag;
        if (!vertexDrag) {
            // Drag ended - signal path changed so downstream can recalculate
            emit pathChanged();
        }
        emit vertexDragChanged(vertexDrag);
    }
}

QGeoCoordinate QGCMapPolyline::vertexCoordinate(int vertex) const
{
    if (vertex >= 0 && vertex < _polylinePath.count()) {
        return _polylinePath[vertex].value<QGeoCoordinate>();
    } else {
        qCWarning(QGCMapPolylineLog) << "QGCMapPolyline::vertexCoordinate bad vertex requested";
        return QGeoCoordinate();
    }
}

QList<QPointF> QGCMapPolyline::nedPolyline(void)
{
    QList<QPointF>  nedPolyline;

    if (count() > 0) {
        QGeoCoordinate  tangentOrigin = vertexCoordinate(0);

        for (int i=0; i<_polylinePath.count(); i++) {
            double y, x, down;
            QGeoCoordinate vertex = vertexCoordinate(i);
            if (i == 0) {
                // This avoids a nan calculation that comes out of convertGeoToNed
                x = y = 0;
            } else {
                QGCGeo::convertGeoToNed(vertex, tangentOrigin, y, x, down);
            }
            nedPolyline += QPointF(x, y);
        }
    }

    return nedPolyline;
}

QList<QGeoCoordinate> QGCMapPolyline::offsetPolyline(double distance)
{
    const QList<QPointF> offsetPoints = QGCPolygonClipper::offsetPolyline(nedPolyline(), distance);
    if (offsetPoints.isEmpty()) {
        return {};
    }

    const QGeoCoordinate tangentOrigin = vertexCoordinate(0);
    QList<QGeoCoordinate> result;
    result.reserve(offsetPoints.size());
    for (const QPointF& point : offsetPoints) {
        QGeoCoordinate coordinate;
        QGCGeo::convertNedToGeo(point.y(), point.x(), 0.0, tangentOrigin, coordinate);
        result.append(coordinate);
    }
    return result;
}

bool QGCMapPolyline::loadKMLOrSHPFile(const QString& file)
{
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    if (!ShapeFileHelper::loadPolylinesFromFile(file, polylines, errorString)) {
        QGC::showAppMessage(errorString);
        return false;
    }
    if (polylines.isEmpty()) {
        QGC::showAppMessage(tr("No polylines found in file"));
        return false;
    }
    const QList<QGeoCoordinate>& rgCoords = polylines.first();

    beginReset();
    clear();
    appendVertices(rgCoords);
    endReset();

    return true;
}

void QGCMapPolyline::_polylineModelDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void QGCMapPolyline::_polylineModelCountChanged(int count)
{
    emit countChanged(count);
}


double QGCMapPolyline::length(void) const
{
    double length = 0;

    for (int i=0; i<_polylinePath.count() - 1; i++) {
        QGeoCoordinate from = _polylinePath[i].value<QGeoCoordinate>();
        QGeoCoordinate to = _polylinePath[i+1].value<QGeoCoordinate>();
        length += from.distanceTo(to);
    }

    return length;
}

void QGCMapPolyline::appendVertices(const QList<QGeoCoordinate>& coordinates)
{
    beginReset();

    QList<QObject*> objects;
    for (const QGeoCoordinate& coordinate: coordinates) {
        objects.append(new QGCQGeoCoordinate(coordinate, this));
        _polylinePath.append(QVariant::fromValue(coordinate));
    }
    _polylineModel.append(objects);

    endReset();

    emit pathChanged();
}

void QGCMapPolyline::beginReset(void)
{
    _polylineModel.beginResetModel();
}

void QGCMapPolyline::endReset(void)
{
    _polylineModel.endResetModel();
}

void QGCMapPolyline::setTraceMode(bool traceMode)
{
    if (traceMode != _traceMode) {
        _traceMode = traceMode;
        emit traceModeChanged(traceMode);
    }
}

void QGCMapPolyline::selectVertex(int index)
{
    if(index == _selectedVertexIndex) return;   // do nothing

    if(-1 <= index && index < count()) {
        _selectedVertexIndex = index;
    } else {
        qCWarning(QGCMapPolylineLog) << QStringLiteral("QGCMapPolyline: Selected vertex index (%1) is out of bounds! "
                                     "Polyline vertices indexes range is [%2..%3].").arg(index).arg(0).arg(count()-1);
        _selectedVertexIndex = -1;   // deselect vertex
    }

    emit selectedVertexChanged(_selectedVertexIndex);
}
