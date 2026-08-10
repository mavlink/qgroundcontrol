#include "SurfacePatchModel.h"

#include <QtCore/QDebug>
#include <QtCore/QtMath>
#include <QtGui/QPainter>
#include <algorithm>
#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "HeightSource.h"
#include "SurfaceAnalysis.h"
#include "SurfaceModel.h"
#include "TerrainHeightSource.h"
#include "TileImageSource.h"

SurfacePatchModel::SurfacePatchModel(QObject* parent) : QAbstractListModel(parent) {}

SurfacePatchModel::~SurfacePatchModel() = default;

void SurfacePatchModel::setScene(GeoScene* scene)
{
    if (scene == _scene) {
        return;
    }
    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }
    _scene = scene;
    emit sceneChanged();

    if (_scene) {
        // cameraChanged also fires on camera destruction (scene clears its pointer)
        connect(_scene, &GeoScene::cameraChanged, this, &SurfacePatchModel::_rebuildSurfaceModel);
        connect(_scene, &GeoScene::sceneOriginChanged, this, &SurfacePatchModel::_sceneOriginChanged);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            emit sceneChanged();
            _rebuildSurfaceModel();
        });
    }
    _rebuildSurfaceModel();
}

GeoMapCamera* SurfacePatchModel::_camera() const
{
    return _scene ? _scene->camera() : nullptr;
}

void SurfacePatchModel::setTerrain(bool terrain)
{
    if (terrain == _terrain) {
        return;
    }
    _terrain = terrain;
    emit terrainChanged();
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

void SurfacePatchModel::setMapType(const QString& mapType)
{
    if (mapType == _mapType) {
        return;
    }
    _mapType = mapType;
    emit mapTypeChanged();

    _resetImagery();
    delete _tileSource;
    _tileSource = nullptr;
    if (!_mapType.isEmpty()) {
        _tileSource = new TileImageSource(_mapType, this);
        connect(_tileSource, &TileImageSource::tileImageReady, this, &SurfacePatchModel::_tileImageReady);
        connect(_tileSource, &TileImageSource::tileImageFailed, this, &SurfacePatchModel::_tileImageFailed);
        for (const TileMath::TileKey& key : std::as_const(_keys)) {
            _requestTileImage(key);
        }
    }
}

void SurfacePatchModel::_resetImagery()
{
    if (_tileSource) {
        // Cancelled requests never signal, so iterating the live map is safe
        for (auto it = _imageRequestKey.cbegin(); it != _imageRequestKey.cend(); ++it) {
            _tileSource->cancelRequest(it.key());
        }
    }
    _imageRequestKey.clear();
    _imageRequestByKey.clear();
    _retiredOrder.clear();
    if (!_tileImages.isEmpty()) {
        _tileImages.clear();
        if (!_keys.isEmpty()) {
            emit dataChanged(index(0), index(_keys.count() - 1), {TileImageRole, HasTileImageRole});
        }
    }
}

void SurfacePatchModel::_requestTileImage(const TileMath::TileKey& key)
{
    if (!_tileSource) {
        return;
    }
    const int requestId = _tileSource->requestTileImage(key);
    _imageRequestKey.insert(requestId, key);
    _imageRequestByKey.insert(key, requestId);
}

void SurfacePatchModel::_tileImageReady(int requestId, const QImage& image)
{
    const auto it = _imageRequestKey.constFind(requestId);
    if (it == _imageRequestKey.constEnd()) {
        return;
    }
    const TileMath::TileKey key = it.value();
    _imageRequestKey.erase(it);
    _imageRequestByKey.remove(key);

    const int row = _keys.indexOf(key);
    if (row < 0) {
        return;
    }
    _tileImages.insert(key, image);
    _notifyTileImageChanged(key);
}

void SurfacePatchModel::_notifyTileImageChanged(const TileMath::TileKey& key)
{
    // A delivered tile changes its own row plus the rows whose fallback
    // sources from it: descendants (ancestor crop) and the direct parent
    // (child composite). Rows with their own image are unaffected.
    for (int row = 0; row < _keys.count(); row++) {
        const TileMath::TileKey& rowKey = _keys.at(row);
        bool affected = (rowKey == key);
        if (!affected && !_tileImages.contains(rowKey)) {
            if ((key.zoom > 0) && (rowKey.zoom == (key.zoom - 1))) {
                affected = (rowKey.x == (key.x >> 1)) && (rowKey.y == (key.y >> 1));
            } else if (rowKey.zoom > key.zoom) {
                const int up = rowKey.zoom - key.zoom;
                affected =
                    (up <= kMaxAncestorFallbackLevels) && ((rowKey.x >> up) == key.x) && ((rowKey.y >> up) == key.y);
            }
        }
        if (affected) {
            const QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {TileImageRole, HasTileImageRole});
        }
    }
}

void SurfacePatchModel::_tileImageFailed(int requestId)
{
    // Leave the image null: the delegate keeps its fallback, and patch churn re-requests
    const auto it = _imageRequestKey.constFind(requestId);
    if (it == _imageRequestKey.constEnd()) {
        return;
    }
    _imageRequestByKey.remove(it.value());
    _imageRequestKey.erase(it);
}

void SurfacePatchModel::_rebuildSurfaceModel()
{
    beginResetModel();
    _keys.clear();
    _resetImagery();
    delete _surfaceModel;
    _surfaceModel = nullptr;
    delete _heightSource;
    _heightSource = nullptr;

    GeoMapCamera* const camera = _camera();
    if (camera) {
        if (_debugHills) {
            _heightSource = new DebugHeightSource(this);
        } else if (_terrain) {
            _heightSource = new TerrainHeightSource(this);
        } else {
            _heightSource = new FlatHeightSource(this);
        }
        _surfaceModel = new SurfaceModel(camera, _heightSource, this);
        connect(_surfaceModel, &SurfaceModel::patchAdded, this, &SurfacePatchModel::_patchAdded);
        connect(_surfaceModel, &SurfaceModel::patchReady, this, &SurfacePatchModel::_patchReady);
        connect(_surfaceModel, &SurfaceModel::patchRemoved, this, &SurfacePatchModel::_patchRemoved);
    }
    endResetModel();

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

void SurfacePatchModel::analyzeSurface() const
{
    GeoMapCamera* const camera = _camera();
    if (!_surfaceModel || !camera) {
        qDebug() << "Surface analysis: no surface model active";
        return;
    }

    SurfaceAnalysis::ViewState view;

    // Unproject a dense screen grid to ground points: coverage is checked
    // where the screen actually looks, independent of the model's own
    // visible-region estimate
    const QSizeF viewport = camera->viewportSize();
    const double maxRange = camera->distance() * SurfaceModel::kMaxRangeMultiplier;
    constexpr int kScreenGrid = 24;
    if (!viewport.isEmpty()) {
        for (int row = 0; row < kScreenGrid; row++) {
            for (int col = 0; col < kScreenGrid; col++) {
                const QPointF screenPos(viewport.width() * (col + 0.5) / kScreenGrid,
                                        viewport.height() * (row + 0.5) / kScreenGrid);
                const std::optional<QPointF> ground = camera->groundPointCapped(screenPos, maxRange);
                if (ground) {
                    view.groundSamples.append(*ground);
                }
            }
        }
    }

    // Camera-below-surface check only applies in 3D (in 2D the mesh renders
    // flattened, so comparing against real heights would be wrong)
    if (_scene && (camera->tilt() > 0.0)) {
        view.cameraGround = camera->cameraGroundPosition();
        view.cameraHeight = camera->distance() * std::cos(qDegreesToRadians(camera->tilt()));
        view.heightScale = _scene->verticalScale();
    }

    const SurfaceAnalysis::Report report =
        SurfaceAnalysis::analyze(_surfaceModel->patches(), SurfaceModel::kGridSize, view);
    qDebug().noquote() << report.text();  // user-triggered diagnostic: always emitted
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
    const QPointF origin = _scene ? _scene->sceneOrigin() : QPointF();

    switch (role) {
        case CenterXRole:
            return (TileMath::tileMinCorner(key).x() + (span / 2.0)) - origin.x();
        case CenterYRole:
            return (TileMath::tileMinCorner(key).y() + (span / 2.0)) - origin.y();
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
        case CoveredRole: {
            const auto patch = _surfaceModel->patch(key);
            return patch ? patch->covered : false;
        }
        case TileImageRole: {
            const QImage own = _tileImages.value(key);
            return own.isNull() ? _fallbackImage(key) : own;
        }
        case HasTileImageRole:
            return _tileImages.contains(key) || _fallbackAvailable(key);
        default:
            return {};
    }
}

bool SurfacePatchModel::_fallbackAvailable(const TileMath::TileKey& key) const
{
    for (int childY = 0; childY < 2; childY++) {
        for (int childX = 0; childX < 2; childX++) {
            if (_tileImages.contains(TileMath::TileKey{(key.x * 2) + childX, (key.y * 2) + childY, key.zoom + 1})) {
                return true;
            }
        }
    }
    for (int up = 1; (up <= kMaxAncestorFallbackLevels) && ((key.zoom - up) >= 0); up++) {
        if (_tileImages.contains(TileMath::TileKey{key.x >> up, key.y >> up, key.zoom - up})) {
            return true;
        }
    }
    return false;
}

QImage SurfacePatchModel::_fallbackImage(const TileMath::TileKey& key) const
{
    // Sharper first: composite direct children (LOD coarsening keeps their detail)
    bool anyChild = false;
    for (int childY = 0; (childY < 2) && !anyChild; childY++) {
        for (int childX = 0; (childX < 2) && !anyChild; childX++) {
            anyChild =
                _tileImages.contains(TileMath::TileKey{(key.x * 2) + childX, (key.y * 2) + childY, key.zoom + 1});
        }
    }
    if (anyChild) {
        constexpr int kCanvas = 256;
        QImage canvas(kCanvas, kCanvas, QImage::Format_RGBA8888);
        canvas.fill(QColor(0x3a, 0x40, 0x48));  // missing quadrants stay neutral
        QPainter painter(&canvas);
        for (int childY = 0; childY < 2; childY++) {
            for (int childX = 0; childX < 2; childX++) {
                const QImage child =
                    _tileImages.value(TileMath::TileKey{(key.x * 2) + childX, (key.y * 2) + childY, key.zoom + 1});
                if (!child.isNull()) {
                    // Tile y grows south and image row 0 is north, so child y maps to top half directly
                    painter.drawImage(QRect((childX * kCanvas) / 2, (childY * kCanvas) / 2, kCanvas / 2, kCanvas / 2),
                                      child);
                }
            }
        }
        return canvas;
    }

    // Blurrier: crop the covering region out of the nearest available ancestor
    for (int up = 1; (up <= kMaxAncestorFallbackLevels) && ((key.zoom - up) >= 0); up++) {
        const TileMath::TileKey ancestor{key.x >> up, key.y >> up, key.zoom - up};
        const QImage image = _tileImages.value(ancestor);
        if (image.isNull()) {
            continue;
        }
        const int split = 1 << up;
        const double cellWidth = static_cast<double>(image.width()) / split;
        const double cellHeight = static_cast<double>(image.height()) / split;
        const int cellX = key.x - (ancestor.x << up);
        const int cellY = key.y - (ancestor.y << up);
        return image.copy(QRect(qRound(cellX * cellWidth), qRound(cellY * cellHeight), qMax(1, qRound(cellWidth)),
                                qMax(1, qRound(cellHeight))));
    }
    return {};
}

QHash<int, QByteArray> SurfacePatchModel::roleNames() const
{
    return {
        {CenterXRole, "centerX"}, {SpanRole, "span"},           {CenterYRole, "centerY"},
        {ZoomRole, "zoomLevel"},  {HeightsRole, "heights"},     {ReadyRole, "ready"},
        {CoveredRole, "covered"}, {TileImageRole, "tileImage"}, {HasTileImageRole, "hasTileImage"},
    };
}

void SurfacePatchModel::_patchAdded(const TileMath::TileKey& key)
{
    beginInsertRows(QModelIndex(), _keys.count(), _keys.count());
    _keys.append(key);
    endInsertRows();
    if (_tileSource) {
        if (_tileImages.contains(key)) {
            _retiredOrder.removeOne(key);  // back to live: exempt from eviction again
        } else {
            _requestTileImage(key);
        }
    }
    emit statsChanged();
}

void SurfacePatchModel::_patchReady(const TileMath::TileKey& key)
{
    const int row = _keys.indexOf(key);
    if (row < 0) {
        return;
    }
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {HeightsRole, ReadyRole, CoveredRole});
    // A new ready patch can newly cover other pending rows
    _notifyCoveredMayHaveChanged();
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

    // Retire the delivered image instead of dropping it: it seeds the
    // fallback for patches that replace this one across LOD changes
    if (_tileImages.contains(key)) {
        _retiredOrder.append(key);
        while (_retiredOrder.count() > kMaxRetiredImages) {
            _tileImages.remove(_retiredOrder.takeFirst());
        }
    }
    const auto requestIt = _imageRequestByKey.constFind(key);
    if (requestIt != _imageRequestByKey.constEnd()) {
        _tileSource->cancelRequest(requestIt.value());
        _imageRequestKey.remove(requestIt.value());
        _imageRequestByKey.erase(requestIt);
    }
    // A removed ready patch may have been the cover of pending rows
    _notifyCoveredMayHaveChanged();
    emit statsChanged();
}

void SurfacePatchModel::_notifyCoveredMayHaveChanged()
{
    // Ready rows are never covered, so only pending rows need re-evaluation
    for (int row = 0; row < _keys.count(); row++) {
        const auto patch = _surfaceModel->patch(_keys.at(row));
        if (patch && !patch->ready) {
            const QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {CoveredRole});
        }
    }
}

void SurfacePatchModel::_sceneOriginChanged()
{
    // All scene positions shift with the origin
    if (!_keys.isEmpty()) {
        emit dataChanged(index(0), index(_keys.count() - 1), {CenterXRole, CenterYRole});
    }
}
