#include "QGCMapPathBase.h"

#include <QMetaMethod>
#include <QTimer>

#include "JsonHelper.h"
#include "JsonParsing.h"
#include "QGCApplication.h"
#include "QGCGeo.h"
#include "QGCQGeoCoordinate.h"

QGCMapPathBase::QGCMapPathBase(QObject* parent)
    : QObject(parent)
{
    _init();
}

QGCMapPathBase::QGCMapPathBase(const QGCMapPathBase& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
    _init();
}

QGCMapPathBase::~QGCMapPathBase()
{
    qgcApp()->removeCompressedSignal(QMetaMethod::fromSignal(&QGCMapPathBase::pathChanged));
}

void QGCMapPathBase::_init()
{
    connect(&_model, &QmlObjectListModel::dirtyChanged, this, &QGCMapPathBase::_modelDirtyChanged);
    connect(&_model, &QmlObjectListModel::countChanged, this, &QGCMapPathBase::_modelCountChanged);
    connect(&_model, &QmlObjectListModel::modelReset, this, [this]() { _onModelReset(); });

    connect(this, &QGCMapPathBase::pathChanged, this, [this]() { _onPathChanged(); });
    connect(this, &QGCMapPathBase::countChanged, this, &QGCMapPathBase::isValidChanged);
    connect(this, &QGCMapPathBase::countChanged, this, &QGCMapPathBase::isEmptyChanged);

    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&QGCMapPathBase::pathChanged));
}

const QGCMapPathBase& QGCMapPathBase::operator=(const QGCMapPathBase& other)
{
    clear();

    const QVariantList vertices = other.path();
    QList<QGeoCoordinate> rgCoord;
    for (const QVariant& vertexVar : vertices) {
        rgCoord.append(vertexVar.value<QGeoCoordinate>());
    }
    appendVertices(rgCoord);

    setDirty(true);

    return *this;
}

void QGCMapPathBase::clear()
{
    _clearImpl();

    _model.clearAndDeleteContents();

    emit cleared();

    setDirty(true);
}

void QGCMapPathBase::_clearImpl()
{
    _path.clear();
    emit pathChanged();
}

void QGCMapPathBase::_onModelReset()
{
    emit pathChanged();
}

void QGCMapPathBase::adjustVertex(int vertexIndex, const QGeoCoordinate coordinate)
{
    if ((vertexIndex < 0) || (vertexIndex >= _path.count())) {
        qWarning() << "QGCMapPathBase::adjustVertex bad vertex requested:count" << vertexIndex << _path.count();
        return;
    }

    _path[vertexIndex] = QVariant::fromValue(coordinate);
    _model.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);
    if (!_deferredPathChanged) {
        _deferredPathChanged = true;
        QTimer::singleShot(0, this, [this]() {
            emit pathChanged();
            _deferredPathChanged = false;
        });
    }
    setDirty(true);
}

void QGCMapPathBase::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            _model.setDirty(false);
        }
        emit dirtyChanged(dirty);
    }
}

QGeoCoordinate QGCMapPathBase::_coordFromPointF(const QPointF& point) const
{
    QGeoCoordinate coord;

    if (_path.count() > 0) {
        const QGeoCoordinate tangentOrigin = _path[0].value<QGeoCoordinate>();
        QGCGeo::convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, coord);
    }

    return coord;
}

QPointF QGCMapPathBase::_pointFFromCoord(const QGeoCoordinate& coordinate) const
{
    if (_path.count() > 0) {
        double y, x, down;
        const QGeoCoordinate tangentOrigin = _path[0].value<QGeoCoordinate>();

        QGCGeo::convertGeoToNed(coordinate, tangentOrigin, y, x, down);
        return QPointF(x, -y);
    }

    return QPointF();
}

void QGCMapPathBase::setPath(const QList<QGeoCoordinate>& path)
{
    beginReset();

    _path.clear();
    _model.clearAndDeleteContents();
    for (const QGeoCoordinate& coord : path) {
        _path.append(QVariant::fromValue(coord));
        _model.append(new QGCQGeoCoordinate(coord, this));
    }

    setDirty(true);

    endReset();
}

void QGCMapPathBase::setPath(const QVariantList& path)
{
    beginReset();

    _path = path;
    _model.clearAndDeleteContents();
    for (int i = 0; i < _path.count(); i++) {
        _model.append(new QGCQGeoCoordinate(_path[i].value<QGeoCoordinate>(), this));
    }
    setDirty(true);

    endReset();
}

void QGCMapPathBase::saveToJson(QJsonObject& json)
{
    QJsonValue jsonValue;

    JsonHelper::saveGeoCoordinateArray(_path, false /* writeAltitude*/, jsonValue);
    json.insert(_jsonKey(), jsonValue);
    setDirty(false);
}

bool QGCMapPathBase::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();
    clear();

    if (required) {
        if (!JsonParsing::validateRequiredKeys(json, QStringList(_jsonKey()), errorString)) {
            return false;
        }
    } else if (!json.contains(_jsonKey())) {
        return true;
    }

    if (!JsonHelper::loadGeoCoordinateArray(json[_jsonKey()], false /* altitudeRequired */, _path, errorString)) {
        return false;
    }

    for (int i = 0; i < _path.count(); i++) {
        _model.append(new QGCQGeoCoordinate(_path[i].value<QGeoCoordinate>(), this));
    }

    setDirty(false);
    emit pathChanged();

    return true;
}

QList<QGeoCoordinate> QGCMapPathBase::coordinateList() const
{
    QList<QGeoCoordinate> coords;

    for (int i = 0; i < _path.count(); i++) {
        coords.append(_path[i].value<QGeoCoordinate>());
    }

    return coords;
}

void QGCMapPathBase::splitSegment(int vertexIndex)
{
    if ((vertexIndex < 0) || (vertexIndex >= _path.length())) {
        qWarning() << "QGCMapPathBase::splitSegment bad vertex requested:count" << vertexIndex << _path.length();
        return;
    }

    int nextIndex = vertexIndex + 1;
    if (nextIndex > _path.length() - 1) {
        if (_closedPath()) {
            nextIndex = 0;
        } else {
            return;
        }
    }

    const QGeoCoordinate firstVertex = _path[vertexIndex].value<QGeoCoordinate>();
    const QGeoCoordinate nextVertex = _path[nextIndex].value<QGeoCoordinate>();

    const double distance = firstVertex.distanceTo(nextVertex);
    const double azimuth = firstVertex.azimuthTo(nextVertex);
    const QGeoCoordinate newVertex = firstVertex.atDistanceAndAzimuth(distance / 2, azimuth);

    if (nextIndex == 0) {
        appendVertex(newVertex);
    } else {
        _model.insert(nextIndex, new QGCQGeoCoordinate(newVertex, this));
        _path.insert(nextIndex, QVariant::fromValue(newVertex));
        emit pathChanged();
        if (0 <= _selectedVertexIndex && vertexIndex < _selectedVertexIndex) {
            selectVertex(_selectedVertexIndex + 1);
        }
        setDirty(true);
    }
}

void QGCMapPathBase::appendVertex(const QGeoCoordinate& coordinate)
{
    _path.append(QVariant::fromValue(coordinate));
    _model.append(new QGCQGeoCoordinate(coordinate, this));
    if (!_deferredPathChanged) {
        _deferredPathChanged = true;
        QTimer::singleShot(0, this, [this]() {
            emit pathChanged();
            _deferredPathChanged = false;
        });
    }
    setDirty(true);
}

void QGCMapPathBase::appendVertices(const QList<QGeoCoordinate>& coordinates)
{
    QList<QObject*> objects;

    beginReset();
    for (const QGeoCoordinate& coordinate : coordinates) {
        objects.append(new QGCQGeoCoordinate(coordinate, this));
        _path.append(QVariant::fromValue(coordinate));
    }
    _model.append(objects);
    endReset();

    emit pathChanged();
    setDirty(true);
}

void QGCMapPathBase::removeVertex(int vertexIndex)
{
    if (vertexIndex < 0 || vertexIndex > _path.length() - 1) {
        qWarning() << "Call to removeVertex with bad vertexIndex:count" << vertexIndex << _path.length();
        return;
    }

    if (_path.length() <= _minVertexCount()) {
        return;
    }

    QObject* const coordObj = _model.removeAt(vertexIndex);
    coordObj->deleteLater();
    if (vertexIndex == _selectedVertexIndex) {
        selectVertex(-1);
    } else if (vertexIndex < _selectedVertexIndex) {
        selectVertex(_selectedVertexIndex - 1);
    }

    _path.removeAt(vertexIndex);
    emit pathChanged();
    setDirty(true);
}

void QGCMapPathBase::_modelDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void QGCMapPathBase::_modelCountChanged(int count)
{
    emit countChanged(count);
}

QGeoCoordinate QGCMapPathBase::vertexCoordinate(int vertex) const
{
    if (vertex >= 0 && vertex < _path.count()) {
        return _path[vertex].value<QGeoCoordinate>();
    } else {
        qWarning() << "QGCMapPathBase::vertexCoordinate bad vertex requested:count" << vertex << _path.count();
        return QGeoCoordinate();
    }
}

QList<QPointF> QGCMapPathBase::nedPath() const
{
    QList<QPointF> nedPath;

    if (count() > 0) {
        const QGeoCoordinate tangentOrigin = vertexCoordinate(0);

        for (int i = 0; i < _model.count(); i++) {
            double y, x, down;
            const QGeoCoordinate vertex = vertexCoordinate(i);
            if (i == 0) {
                x = y = 0;
            } else {
                QGCGeo::convertGeoToNed(vertex, tangentOrigin, y, x, down);
            }
            nedPath += QPointF(x, y);
        }
    }

    return nedPath;
}

void QGCMapPathBase::setInteractive(bool interactive)
{
    if (_interactive != interactive) {
        _interactive = interactive;
        emit interactiveChanged(interactive);
    }
}

void QGCMapPathBase::setTraceMode(bool traceMode)
{
    if (traceMode != _traceMode) {
        _traceMode = traceMode;
        emit traceModeChanged(traceMode);
    }
}

void QGCMapPathBase::selectVertex(int index)
{
    if (index == _selectedVertexIndex) return;

    if (-1 <= index && index < count()) {
        _selectedVertexIndex = index;
    } else {
        if (!qgcApp()->runningUnitTests()) {
            qWarning() << QStringLiteral("QGCMapPathBase: Selected vertex index (%1) is out of bounds! "
                                         "Vertex indexes range is [%2..%3].")
                              .arg(index)
                              .arg(0)
                              .arg(count() - 1);
        }
        _selectedVertexIndex = -1;
    }

    emit selectedVertexChanged(_selectedVertexIndex);
}

bool QGCMapPathBase::loadShapeFile(const QString& file)
{
    QString errorString;
    QList<QList<QGeoCoordinate>> shapes;
    if (!_loadFromFile(file, shapes, errorString)) {
        qgcApp()->showAppMessage(errorString);
        return false;
    }
    if (shapes.isEmpty()) {
        qgcApp()->showAppMessage(_emptyFileMessage());
        return false;
    }
    const QList<QGeoCoordinate>& rgCoords = shapes.first();

    clear();
    appendVertices(rgCoords);

    return true;
}

void QGCMapPathBase::beginReset()
{
    _model.beginResetModel();
}

void QGCMapPathBase::endReset()
{
    _model.endResetModel();
}
