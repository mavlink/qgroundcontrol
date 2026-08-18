#include "SurfacePatchModel.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QtMath>
#include <QtGui/QPainter>

#include <algorithm>
#include <cmath>
#include <utility>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "HeightField.h"
#include "HeightSource.h"
#include "QGCLoggingCategory.h"
#include "SurfaceAnalysis.h"
#include "SurfaceModel.h"
#include "TerrariumTileFetcher.h"
#include "TileImageSource.h"

QGC_LOGGING_CATEGORY(GeoMapSurfacePatchModelLog, "GeoMap.SurfacePatchModel")
QGC_LOGGING_CATEGORY(GeoMapSurfacePatchModelVerboseLog, "GeoMap.SurfacePatchModel.Verbose")

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

void SurfacePatchModel::_scheduleStatsChanged()
{
    // Patch events arrive in bursts (up to hundreds/s during churn); one
    // emit per event-loop pass bounds the QML binding re-evaluations
    if (_statsNotifyPending) {
        return;
    }
    _statsNotifyPending = true;
    QTimer::singleShot(0, this, [this] {
        _statsNotifyPending = false;
        emit statsChanged();
    });
}

#ifdef QGC_UNITTEST_BUILD
void SurfacePatchModel::drainUpdates()
{
    if (_surfaceModel) {
        _surfaceModel->drainUpdates();
    }
}
#endif

void SurfacePatchModel::setTerrain(bool terrain)
{
    if (terrain == _terrain) {
        return;
    }
    qCDebug(GeoMapSurfacePatchModelLog) << "terrain" << _terrain << "->" << terrain;
    _terrain = terrain;
    emit terrainChanged();
    _rebuildSurfaceModel();
}

void SurfacePatchModel::setDebugHills(bool debugHills)
{
    if (debugHills == _debugHills) {
        return;
    }
    qCDebug(GeoMapSurfacePatchModelLog) << "debugHills" << _debugHills << "->" << debugHills;
    _debugHills = debugHills;
    emit debugHillsChanged();
    _rebuildSurfaceModel();
}

void SurfacePatchModel::setMapType(const QString& mapType)
{
    if (mapType == _mapType) {
        return;
    }
    qCDebug(GeoMapSurfacePatchModelLog) << "mapType" << _mapType << "->" << mapType;
    _mapType = mapType;
    emit mapTypeChanged();

    _resetImagery();
    delete _tileSource;
    _tileSource = nullptr;
    if (!_mapType.isEmpty()) {
        _tileSource = new TileImageSource(_mapType, this, _tileImageNetworkManager);
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
    _failedImageKeys.clear();
    if (_imageRetryTimer) {
        _imageRetryTimer->stop();
    }
    _retiredOrder.clear();
    _fallbackCache.clear();
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
    _fallbackCache.remove(key);  // own image supersedes any cached miss
    qCDebug(GeoMapSurfacePatchModelVerboseLog) << "tile image ready for" << key;
    _invalidateFallbacks(key);
    _notifyTileImageChanged(key);
}

void SurfacePatchModel::_invalidateFallbacks(const TileMath::TileKey& tile)
{
    // A patch's fallback sources from its direct children and its ancestors
    // (up to kMaxAncestorFallbackLevels), so a changed tile invalidates its
    // parent's composite and every cached descendant's crop. Fallbacks read
    // only delivered images, never other fallbacks, so invalidation never
    // cascades further.
    if (tile.zoom > TileMath::kMinZoom) {
        _fallbackCache.remove(TileMath::TileKey{tile.x >> 1, tile.y >> 1, tile.zoom - 1});
    }
    for (auto it = _fallbackCache.begin(); it != _fallbackCache.end();) {
        const int down = it.key().zoom - tile.zoom;
        if ((down > 0) && (down <= kMaxAncestorFallbackLevels) && ((it.key().x >> down) == tile.x) &&
            ((it.key().y >> down) == tile.y)) {
            it = _fallbackCache.erase(it);
        } else {
            ++it;
        }
    }
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
            if ((key.zoom > TileMath::kMinZoom) && (rowKey.zoom == (key.zoom - 1))) {
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
    const auto it = _imageRequestKey.constFind(requestId);
    if (it == _imageRequestKey.constEnd()) {
        return;
    }
    const TileMath::TileKey key = it.value();
    _imageRequestByKey.remove(key);
    _imageRequestKey.erase(it);

    // The delegate keeps its fallback meanwhile; a resident patch retries on
    // the paced timer (patch churn cannot be relied on to re-request - a
    // patch that stays resident would keep the loading fallback forever)
    if (!_keys.contains(key)) {
        return;
    }
    qCDebug(GeoMapSurfacePatchModelLog) << "tile image failed for" << key << "(retry scheduled)";
    _failedImageKeys.insert(key);
    if (!_imageRetryTimer) {
        _imageRetryTimer = new QTimer(this);
        _imageRetryTimer->setSingleShot(true);
        _imageRetryTimer->setInterval(kImageRetryMs);
        connect(_imageRetryTimer, &QTimer::timeout, this, &SurfacePatchModel::_retryFailedImages);
    }
    if (!_imageRetryTimer->isActive()) {
        _imageRetryTimer->start();
    }
}

void SurfacePatchModel::_retryFailedImages()
{
    const QSet<TileMath::TileKey> failed = std::exchange(_failedImageKeys, {});
    for (const TileMath::TileKey& key : failed) {
        // Re-check residency and in-flight state: churn may have re-requested
        // or removed the patch since the failure
        if (!_keys.contains(key) || _tileImages.contains(key) || _imageRequestByKey.contains(key)) {
            continue;
        }
        _requestTileImage(key);
    }
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
    delete _heightField;
    _heightField = nullptr;

    GeoMapCamera* const camera = _camera();
    if (camera) {
        if (_debugHills) {
            _heightSource = new DebugHeightSource(this);
        } else if (_terrain) {
            _heightSource = new TerrariumTileFetcher(this);
        } else {
            _heightSource = new FlatHeightSource(this);
        }
        qCDebug(GeoMapSurfacePatchModelLog) << "rebuilding surface model, height source:"
                                            << (_debugHills ? "debug hills" : (_terrain ? "terrain" : "flat"));
        _heightField = new HeightField(this);
        _heightSource->setHeightField(_heightField);
        connect(_heightField, &HeightField::regionChanged, this, &SurfacePatchModel::terrainHeightsChanged);
        _surfaceModel = new SurfaceModel(camera, _heightSource, _heightField, this);
        connect(_surfaceModel, &SurfaceModel::patchAdded, this, &SurfacePatchModel::_patchAdded);
        connect(_surfaceModel, &SurfaceModel::patchMeshChanged, this, &SurfacePatchModel::_patchReady);
        connect(_surfaceModel, &SurfaceModel::patchEdgeDeltasChanged, this,
                &SurfacePatchModel::_patchEdgeDeltasChanged);
        connect(_surfaceModel, &SurfaceModel::patchRemoved, this, &SurfacePatchModel::_patchRemoved);
    }
    endResetModel();

    if (_surfaceModel) {
        _surfaceModel->update();
    }
    emit statsChanged();
    // Field replacement invalidates every previous height answer
    emit terrainHeightsChanged();
}

double SurfacePatchModel::terrainHeightAt(const QGeoCoordinate& coordinate) const
{
    // Invalid coordinates carry NaN lat/lon, which the height lookup cannot digest
    if (!_heightField || !coordinate.isValid()) {
        return 0.0;
    }
    return _heightField->heightAt(TileMath::geoToWorld(coordinate));
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

// GCOVR_EXCL_START — perf-stats/capture/diagnostics instrumentation, not shipping behavior
void SurfacePatchModel::_startStatsSampling()
{
    if (!_statsTimer) {
        _statsTimer = new QTimer(this);
        _statsTimer->setInterval(1000);
        connect(_statsTimer, &QTimer::timeout, this, &SurfacePatchModel::_statsTick);
    }
    _statAdds = 0;
    _statRemoves = 0;
    _statComposites = 0;
    if (_surfaceModel) {
        _surfaceModel->takeUpdateStats();  // discard pre-window accumulation
    }
    _statsTimer->start();
}

void SurfacePatchModel::setStatsEnabled(bool enabled)
{
    if (enabled == _statsEnabled) {
        return;
    }
    _statsEnabled = enabled;
    emit statsEnabledChanged();

    // The sampler is shared with capture recording, which owns it while
    // capturing: the overlay toggling must not reset or stop a live capture
    if (_statsEnabled) {
        if (!_capturing) {
            _startStatsSampling();
        }
    } else if (!_capturing && _statsTimer) {
        _statsTimer->stop();
    }
    _statsText.clear();
    emit statsTextChanged();
}

void SurfacePatchModel::_statsTick()
{
    const SurfaceModel::UpdateStats stats =
        _surfaceModel ? _surfaceModel->takeUpdateStats() : SurfaceModel::UpdateStats{};
    const double avgMs = stats.updates ? (stats.totalUs / 1000.0) / stats.updates : 0.0;
    const double maxMs = stats.maxUs / 1000.0;
    const double addAvgMs = stats.updates ? (stats.addTotalUs / 1000.0) / stats.updates : 0.0;
    const double addMaxMs = stats.addMaxUs / 1000.0;
    _statsText = QStringLiteral("upd/s %1  avg %2 ms  max %3 ms  |  patch +%4 -%5 /s  comp/s %6")
                     .arg(stats.updates)
                     .arg(avgMs, 0, 'f', 2)
                     .arg(maxMs, 0, 'f', 2)
                     .arg(_statAdds)
                     .arg(_statRemoves)
                     .arg(_statComposites);
    if (_capturing) {
        _captureWorstMaxMs = std::max(_captureWorstMaxMs, maxMs);
        _captureWorstAvgMs = std::max(_captureWorstAvgMs, avgMs);
        GeoMapCamera* const camera = _camera();
        _captureRows.append(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14")
                                .arg(_captureClock.elapsed() / 1000.0, 0, 'f', 1)
                                .arg(stats.updates)
                                .arg(avgMs, 0, 'f', 3)
                                .arg(maxMs, 0, 'f', 3)
                                .arg(addAvgMs, 0, 'f', 3)
                                .arg(addMaxMs, 0, 'f', 3)
                                .arg(_statAdds)
                                .arg(_statRemoves)
                                .arg(_statComposites)
                                .arg(patchCount())
                                .arg(pendingCount())
                                .arg(maxZoomLevel())
                                .arg(camera ? camera->distance() : 0.0, 0, 'f', 0)
                                .arg(camera ? camera->tilt() : 0.0, 0, 'f', 1));
    }
    _statAdds = 0;
    _statRemoves = 0;
    _statComposites = 0;
    emit statsTextChanged();
}

void SurfacePatchModel::startCapture()
{
    if (_capturing) {
        return;
    }
    _capturing = true;
    emit capturingChanged();

    _captureRows.clear();
    _captureWorstMaxMs = 0.0;
    _captureWorstAvgMs = 0.0;
    _captureRows.append(
        QStringLiteral("time_s,updates,avg_ms,max_ms,add_avg_ms,add_max_ms,patch_adds,patch_removes,composites,"
                       "patches,pending,max_zoom,cam_dist_m,cam_tilt_deg"));
    _captureClock.start();
    // Run the sampler directly rather than via statsEnabled: that property is
    // QML-bound to the overlay toggle, and writing it here would fight the
    // binding (and leave the sampler running after the capture ends)
    _startStatsSampling();
}

QString SurfacePatchModel::stopCapture()
{
    if (!_capturing) {
        return QString();
    }
    _statsTick();  // flush the partial interval: the final settle pass must reach the CSV and worst-case values
    _capturing = false;
    emit capturingChanged();
    if (!_statsEnabled && _statsTimer) {
        _statsTimer->stop();
    }

    const QString path = QDir::temp().filePath(
        QStringLiteral("qgc-geomap-capture-%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "GeoMap capture: cannot write" << path;
        _statsText = QStringLiteral("capture failed: %1").arg(path);
        emit statsTextChanged();
        return QString();
    }
    const QByteArray data = _captureRows.join(QLatin1Char('\n')).toUtf8() + '\n';
    const bool written = (file.write(data) == data.size()) && file.flush();
    file.close();
    _captureRows.clear();
    if (!written) {
        (void) file.remove();  // never hand back a truncated CSV
        qWarning() << "GeoMap capture: write failed" << path;
        _statsText = QStringLiteral("capture failed: %1").arg(path);
        emit statsTextChanged();
        return QString();
    }

#ifdef Q_OS_ANDROID
    // Non-debuggable release builds hide the app cache from adb; dump the CSV
    // to logcat so it can be pulled with: adb logcat -d -s GeoMapCapture
    for (const QByteArray& line : data.split('\n')) {
        if (!line.isEmpty()) {
            __android_log_print(ANDROID_LOG_WARN, "GeoMapCapture", "%s", line.constData());
        }
    }
#endif

    // No log output on success: the overlay shows the path, and UI tests run
    // with strict log checking. The worst-case summary makes the overlay a
    // one-glance verdict without reading the CSV.
    _statsText = QStringLiteral("capture: worst max %1 ms  worst avg %2 ms  ->  %3")
                     .arg(_captureWorstMaxMs, 0, 'f', 2)
                     .arg(_captureWorstAvgMs, 0, 'f', 2)
                     .arg(path);
    emit statsTextChanged();
    return path;
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
        view.cameraHeight =
            camera->centerElevation() + (camera->distance() * std::cos(qDegreesToRadians(camera->tilt())));
        view.heightScale = _scene->verticalScale();
    }

    const SurfaceAnalysis::Report report =
        SurfaceAnalysis::analyze(_surfaceModel->patches(), SurfaceModel::kGridSize, view);
    qDebug().noquote() << report.text();  // user-triggered diagnostic: always emitted
}

// GCOVR_EXCL_STOP

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
        case TileXRole:
            return key.x;
        case TileYRole:
            return key.y;
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
        case EdgeLodDeltasRole:
            return QVariant::fromValue(_surfaceModel->edgeLodDeltas(key));
        case TileImageRole: {
            const QImage own = _tileImages.value(key);
            if (!own.isNull()) {
                return own;
            }
            const auto cached = _fallbackCache.constFind(key);
            if (cached != _fallbackCache.constEnd()) {
                return *cached;
            }
            const QImage fallback = _fallbackImage(key);
            _fallbackCache.insert(key, fallback);
            return fallback;
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
    QImage image = _compositeFromChildren(key);
    if (image.isNull()) {
        // Blurrier: crop the covering region out of the nearest available ancestor
        image = _cropFromAncestor(key);
    }
    return image;
}

QImage SurfacePatchModel::_compositeFromChildren(const TileMath::TileKey& key) const
{
    bool anyChild = false;
    for (int childY = 0; (childY < 2) && !anyChild; childY++) {
        for (int childX = 0; (childX < 2) && !anyChild; childX++) {
            anyChild =
                _tileImages.contains(TileMath::TileKey{(key.x * 2) + childX, (key.y * 2) + childY, key.zoom + 1});
        }
    }
    if (!anyChild) {
        return {};
    }
    constexpr int kCanvas = 256;
    _statComposites++;
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

QImage SurfacePatchModel::_cropFromAncestor(const TileMath::TileKey& key) const
{
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
        _statComposites++;
        return image.copy(QRect(qRound(cellX * cellWidth), qRound(cellY * cellHeight), qMax(1, qRound(cellWidth)),
                                qMax(1, qRound(cellHeight))));
    }
    return {};
}

QHash<int, QByteArray> SurfacePatchModel::roleNames() const
{
    return {
        {CenterXRole, "centerX"},
        {SpanRole, "span"},
        {CenterYRole, "centerY"},
        {ZoomRole, "zoomLevel"},
        {HeightsRole, "heights"},
        {ReadyRole, "ready"},
        {CoveredRole, "covered"},
        {TileImageRole, "tileImage"},
        {HasTileImageRole, "hasTileImage"},
        {EdgeLodDeltasRole, "edgeLodDeltas"},
        {TileXRole, "tileX"},
        {TileYRole, "tileY"},
    };
}

void SurfacePatchModel::_patchAdded(const TileMath::TileKey& key)
{
    beginInsertRows(QModelIndex(), _keys.count(), _keys.count());
    _keys.append(key);
    endInsertRows();
    _statAdds++;
    if (_tileSource) {
        if (_tileImages.contains(key)) {
            _retiredOrder.removeOne(key);  // back to live: exempt from eviction again
        } else {
            _requestTileImage(key);
        }
    }
    _scheduleStatsChanged();
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
    _notifyCoveredMayHaveChanged(key);
    _scheduleStatsChanged();
}

void SurfacePatchModel::_patchEdgeDeltasChanged(const TileMath::TileKey& key)
{
    const int row = _keys.indexOf(key);
    if (row < 0) {
        return;
    }
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {EdgeLodDeltasRole});
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
    _statRemoves++;

    // Retire the delivered image instead of dropping it: it seeds the
    // fallback for patches that replace this one across LOD changes
    if (_tileImages.contains(key)) {
        _retiredOrder.append(key);
        while (_retiredOrder.count() > kMaxRetiredImages) {
            const TileMath::TileKey evicted = _retiredOrder.takeFirst();
            _tileImages.remove(evicted);
            _invalidateFallbacks(evicted);
        }
    }
    _fallbackCache.remove(key);  // row gone: keep the cache bounded by live rows
    _failedImageKeys.remove(key);
    const auto requestIt = _imageRequestByKey.constFind(key);
    if (requestIt != _imageRequestByKey.constEnd()) {
        _tileSource->cancelRequest(requestIt.value());
        _imageRequestKey.remove(requestIt.value());
        _imageRequestByKey.erase(requestIt);
    }
    // A removed ready patch may have been the cover of pending rows
    _notifyCoveredMayHaveChanged(key);
    _scheduleStatsChanged();
}

void SurfacePatchModel::_notifyCoveredMayHaveChanged(const TileMath::TileKey& changedKey)
{
    // Ready rows are never covered, and only rows overlapping the changed
    // patch can gain/lose their cover. The rect prefilter keeps this O(rows)
    // per event; unscoped notification was O(rows^2) via the CoveredRole
    // re-evaluations and dominated passes at high resident counts.
    const double span = TileMath::tileSpanAtZoom(changedKey.zoom);
    const QRectF changedRect(TileMath::tileMinCorner(changedKey), QSizeF(span, span));
    for (int row = 0; row < _keys.count(); row++) {
        const TileMath::TileKey& rowKey = _keys.at(row);
        const double rowSpan = TileMath::tileSpanAtZoom(rowKey.zoom);
        if (!changedRect.intersects(QRectF(TileMath::tileMinCorner(rowKey), QSizeF(rowSpan, rowSpan)))) {
            continue;
        }
        const auto patch = _surfaceModel->patch(rowKey);
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
