#include "SurfacePatchModel.h"

#include <algorithm>

#include "GeoViewCamera.h"
#include "HeightSource.h"
#include "SurfaceModel.h"

SurfacePatchModel::SurfacePatchModel(QObject* parent) : QAbstractListModel(parent) {}

SurfacePatchModel::~SurfacePatchModel() = default;

void SurfacePatchModel::setCamera(GeoViewCamera* camera)
{
    if (camera == _camera) {
        return;
    }
    if (_camera) {
        disconnect(_camera, nullptr, this, nullptr);
    }
    _camera = camera;
    _originSet = false;  // new camera anchors fresh; a stale origin breaks float precision silently
    emit cameraChanged();

    if (_camera) {
        connect(_camera, &GeoViewCamera::centerChanged, this, &SurfacePatchModel::_maybeReanchor);
        connect(_camera, &QObject::destroyed, this, [this] {
            _camera = nullptr;
            _originSet = false;
            _rebuildSurfaceModel();
        });
    }
    _rebuildSurfaceModel();
}

void SurfacePatchModel::setDebugHills(bool debugHills)
{
    if (debugHills == _debugHills) {
        return;
    }
    _debugHills = debugHills;
    emit debugHillsChanged();
    _rebuildSurfaceModel();
}

void SurfacePatchModel::_rebuildSurfaceModel()
{
    beginResetModel();
    _keys.clear();
    delete _surfaceModel;
    _surfaceModel = nullptr;
    delete _heightSource;
    _heightSource = nullptr;

    if (_camera) {
        _heightSource = _debugHills ? static_cast<HeightSource*>(new DebugHeightSource(this))
                                    : static_cast<HeightSource*>(new FlatHeightSource(this));
        _surfaceModel = new SurfaceModel(_camera, _heightSource, this);
        connect(_surfaceModel, &SurfaceModel::patchAdded, this, &SurfacePatchModel::_patchAdded);
        connect(_surfaceModel, &SurfaceModel::patchReady, this, &SurfacePatchModel::_patchReady);
        connect(_surfaceModel, &SurfaceModel::patchRemoved, this, &SurfacePatchModel::_patchRemoved);
    }
    endResetModel();

    if (_camera && !_originSet) {
        _sceneOrigin = TileMath::geoToWorld(_camera->center());
        _originSet = true;
        emit sceneOriginChanged();
    }
    if (_surfaceModel) {
        _surfaceModel->update();
    }
    emit statsChanged();
}

int SurfacePatchModel::gridSize() const
{
    return SurfaceModel::kGridSize;
}

double SurfacePatchModel::maxRangeMultiplier() const
{
    return SurfaceModel::kMaxRangeMultiplier;
}

qreal SurfacePatchModel::verticalScale() const
{
    return TileMath::mercatorScale(TileMath::worldToGeo(_sceneOrigin).latitude());
}

int SurfacePatchModel::pendingCount() const
{
    return _surfaceModel ? _surfaceModel->pendingCount() : 0;
}

int SurfacePatchModel::maxZoomLevel() const
{
    int maxZoom = 0;
    for (const TileMath::TileKey& key : _keys) {
        maxZoom = std::max(maxZoom, key.zoom);
    }
    return maxZoom;
}

int SurfacePatchModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _keys.count();
}

QVariant SurfacePatchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || (index.row() < 0) || (index.row() >= _keys.count()) || !_surfaceModel) {
        return {};
    }

    const TileMath::TileKey& key = _keys.at(index.row());
    const double span = TileMath::tileSpanAtZoom(key.zoom);

    switch (role) {
        case CenterXRole:
            return (TileMath::tileMinCorner(key).x() + (span / 2.0)) - _sceneOrigin.x();
        case CenterYRole:
            return (TileMath::tileMinCorner(key).y() + (span / 2.0)) - _sceneOrigin.y();
        case SpanRole:
            return span;
        case ZoomRole:
            return key.zoom;
        case HeightsRole: {
            const auto patch = _surfaceModel->patch(key);
            return patch ? QVariant::fromValue(patch->heights) : QVariant::fromValue(QList<float>());
        }
        case ReadyRole: {
            const auto patch = _surfaceModel->patch(key);
            return patch ? patch->ready : false;
        }
        default:
            return {};
    }
}

QHash<int, QByteArray> SurfacePatchModel::roleNames() const
{
    return {
        {CenterXRole, "centerX"}, {SpanRole, "span"},       {CenterYRole, "centerY"},
        {ZoomRole, "zoomLevel"},  {HeightsRole, "heights"}, {ReadyRole, "ready"},
    };
}

void SurfacePatchModel::_patchAdded(const TileMath::TileKey& key)
{
    beginInsertRows(QModelIndex(), _keys.count(), _keys.count());
    _keys.append(key);
    endInsertRows();
    emit statsChanged();
}

void SurfacePatchModel::_patchReady(const TileMath::TileKey& key)
{
    const int row = _keys.indexOf(key);
    if (row < 0) {
        return;
    }
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {HeightsRole, ReadyRole});
    emit statsChanged();
}

void SurfacePatchModel::_patchRemoved(const TileMath::TileKey& key)
{
    const int row = _keys.indexOf(key);
    if (row < 0) {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    _keys.removeAt(row);
    endRemoveRows();
    emit statsChanged();
}

void SurfacePatchModel::_maybeReanchor()
{
    if (!_camera) {
        return;
    }
    const QPointF cameraWorld = TileMath::geoToWorld(_camera->center());
    if (!_originSet) {
        _sceneOrigin = cameraWorld;
        _originSet = true;
        emit sceneOriginChanged();
        return;
    }
    const QPointF offset = cameraWorld - _sceneOrigin;
    if (QPointF::dotProduct(offset, offset) < (kReanchorDistance * kReanchorDistance)) {
        return;
    }
    _sceneOrigin = cameraWorld;
    emit sceneOriginChanged();
    // All scene positions shift with the origin
    if (!_keys.isEmpty()) {
        emit dataChanged(index(0), index(_keys.count() - 1), {CenterXRole, CenterYRole});
    }
}
