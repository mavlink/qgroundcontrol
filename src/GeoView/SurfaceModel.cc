#include "SurfaceModel.h"

#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QtMath>
#include <cmath>
#include <queue>
#include <vector>

#include "GeoViewCamera.h"
#include "HeightSource.h"

namespace {

QRectF patchRect(const TileMath::TileKey& key)
{
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const QPointF minCorner = TileMath::tileMinCorner(key);
    return QRectF(minCorner.x(), minCorner.y(), span, span);
}

}  // namespace

SurfaceModel::SurfaceModel(GeoViewCamera* camera, HeightSource* heightSource, QObject* parent)
    : QObject(parent), _camera(camera), _heightSource(heightSource)
{
    qRegisterMetaType<TileMath::TileKey>();

    connect(_heightSource, &HeightSource::patchHeightsReady, this, &SurfaceModel::_heightsReady);
    connect(_heightSource, &HeightSource::patchHeightsFailed, this, &SurfaceModel::_heightsFailed);

    connect(_camera, &GeoViewCamera::centerChanged, this, &SurfaceModel::update);
    connect(_camera, &GeoViewCamera::headingChanged, this, &SurfaceModel::update);
    connect(_camera, &GeoViewCamera::tiltChanged, this, &SurfaceModel::update);
    connect(_camera, &GeoViewCamera::distanceChanged, this, &SurfaceModel::update);
    connect(_camera, &GeoViewCamera::viewportSizeChanged, this, &SurfaceModel::update);
    connect(_camera, &GeoViewCamera::fieldOfViewChanged, this, &SurfaceModel::update);
}

void SurfaceModel::update()
{
    if (!_camera->isPositioned()) {
        return;  // the default pose is null island; building patches there fires doomed terrain fetches
    }
    if (_camera->viewportSize().isEmpty()) {
        return;  // keep the last valid set; transient 0-size viewports occur during layout
    }

    // Max terrain height scans every resident vertex: compute once per update
    const double terrainZ = _maxTerrainZ();
    const QRectF visible = _visibleGroundRect(terrainZ);
    _culledTerrainZ = terrainZ;
    const QPointF cameraGround = _camera->cameraGroundPosition();
    const double cameraHeight = _camera->distance() * std::cos(qDegreesToRadians(_camera->tilt()));

    const QList<TileMath::TileKey> desired = _desiredPatches(visible, cameraGround, cameraHeight);

    QSet<TileMath::TileKey> desiredSet(desired.cbegin(), desired.cend());

    // Drop pending patches no longer desired, cancelling in-flight height
    // requests. Ready patches being replaced retire instead: they keep
    // rendering until their replacements have heights, so LOD churn and
    // panning never open holes in the surface.
    for (auto it = _patches.begin(); it != _patches.end();) {
        if (desiredSet.contains(it.key())) {
            it.value().retiring = false;  // re-desired while retiring: it is ready, keep as-is
            ++it;
            continue;
        }
        if (it.value().retiring) {
            ++it;  // awaiting sweep
            continue;
        }
        if (it.value().ready) {
            it.value().retiring = true;
            ++it;
            continue;
        }
        // Pending patches own a tracked request, except while awaiting a retry
        // (requestId 0, never issued by a source): then both calls are no-ops.
        // _heightsReady untracks the id when a patch becomes ready (guard also
        // protects against a future source reusing ids).
        _heightSource->cancelRequest(it.value().requestId);
        _requestKeys.remove(it.value().requestId);
        const TileMath::TileKey removedKey = it.key();
        it = _patches.erase(it);
        emit patchRemoved(removedKey);
    }

    // Request heights for newly desired patches
    for (const TileMath::TileKey& key : desired) {
        if (_patches.contains(key)) {
            continue;
        }
        PatchData data;
        data.requestId = _heightSource->requestPatchHeights(key, kGridSize);
        _requestKeys.insert(data.requestId, key);
        _patches.insert(key, data);
        emit patchAdded(key);
    }

    _sweepRetiring();
}

QList<SurfaceModel::Patch> SurfaceModel::patches() const
{
    QList<Patch> result;
    result.reserve(_patches.count());
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        result.append(Patch{it.key(), it.value().heights, it.value().ready, _isCovered(it.key(), it.value())});
    }
    return result;
}

std::optional<SurfaceModel::Patch> SurfaceModel::patch(const TileMath::TileKey& key) const
{
    const auto it = _patches.constFind(key);
    if (it == _patches.constEnd()) {
        return std::nullopt;
    }
    return Patch{key, it.value().heights, it.value().ready, _isCovered(key, it.value())};
}

bool SurfaceModel::_isCovered(const TileMath::TileKey& key, const PatchData& data) const
{
    if (data.ready) {
        return false;
    }
    // A pending patch overlapped by ready geometry (its retiring ancestor,
    // descendants, or a degraded flat fallback) is covered: rendering its
    // empty flat grid too would z-fight the cover and occlude terrain below
    // sea level
    const QRectF rect = patchRect(key);
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        if (it.value().ready && (it.key() != key) && rect.intersects(patchRect(it.key()))) {
            return true;
        }
    }
    return false;
}

int SurfaceModel::pendingCount() const
{
    int pending = 0;
    for (const PatchData& data : _patches) {
        if (!data.ready) {
            pending++;
        }
    }
    return pending;
}

QRectF SurfaceModel::_visibleGroundRect(double terrainZ) const
{
    const QSizeF viewport = _camera->viewportSize();
    const double maxRange = _camera->distance() * kMaxRangeMultiplier;

    // Terrain-aware near boundary: rays that hit the ground plane far ahead
    // cross the terrain-top plane much closer to (or behind) the camera, so
    // tall terrain there is visible even though its flat-ground point is not
    const double eyeZ = _camera->distance() * std::cos(qDegreesToRadians(_camera->tilt()));
    const QPointF cameraGround = _camera->cameraGroundPosition();
    const double terrainT = ((terrainZ > 0.0) && (eyeZ > terrainZ)) ? (1.0 - (terrainZ / eyeZ)) : 0.0;

    // Sample a screen grid; horizon misses are capped at maxRange. Accumulate
    // extremes manually (zero-size QRectFs are "null" and united() ignores them).
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
    bool first = true;
    const auto include = [&](const QPointF& point) {
        if (first) {
            minX = maxX = point.x();
            minY = maxY = point.y();
            first = false;
        } else {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());
            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
    };
    for (int row = 0; row < kVisibleSampleGrid; row++) {
        for (int col = 0; col < kVisibleSampleGrid; col++) {
            const QPointF screenPos((viewport.width() * col) / (kVisibleSampleGrid - 1),
                                    (viewport.height() * row) / (kVisibleSampleGrid - 1));
            const auto ground = _camera->groundPointCapped(screenPos, maxRange);
            if (!ground) {
                continue;
            }
            include(*ground);
            if (terrainT > 0.0) {
                include(cameraGround + ((*ground - cameraGround) * terrainT));
            }
        }
    }
    if (first) {
        return QRectF();
    }
    // Always keep the camera's tile in range: the terrain ceiling above only
    // knows resident patches, so terrain living solely in never-requested
    // tiles near the camera could otherwise never be discovered
    include(cameraGround);

    const double half = TileMath::worldSize() / 2.0;
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY))
        .intersected(QRectF(-half, -half, TileMath::worldSize(), TileMath::worldSize()));
}

/// Tallest resident terrain in scene units (heights render scaled by the
/// mercator vertical scale); 0 when nothing has heights yet
double SurfaceModel::_maxTerrainZ() const
{
    float maxHeight = 0.0f;
    for (const PatchData& data : _patches) {
        for (const float height : data.heights) {
            if (std::isfinite(height)) {
                maxHeight = std::max(maxHeight, height);
            }
        }
    }
    if (maxHeight <= 0.0f) {
        return 0.0;
    }
    return maxHeight * TileMath::mercatorScale(_camera->center().latitude());
}

double SurfaceModel::_projectedPixels(const TileMath::TileKey& key, const QPointF& cameraGround,
                                      double cameraHeight) const
{
    const QRectF rect = patchRect(key);
    const double span = rect.width();

    // Distance from the camera to the closest point of the patch (heights ignored for LOD)
    const double dx = std::max({rect.left() - cameraGround.x(), cameraGround.x() - rect.right(), 0.0});
    const double dy = std::max({rect.top() - cameraGround.y(), cameraGround.y() - rect.bottom(), 0.0});
    const double distance = std::hypot(std::hypot(dx, dy), cameraHeight);

    const double metersPerPixel =
        (2.0 * distance * std::tan(qDegreesToRadians(_camera->fieldOfView()) / 2.0)) / _camera->viewportSize().height();
    return span / metersPerPixel;
}

QList<TileMath::TileKey> SurfaceModel::_desiredPatches(const QRectF& visible, const QPointF& cameraGround,
                                                       double cameraHeight) const
{
    // Coverage-preserving refinement: repeatedly split the visible patch with
    // the largest projected size while the budget allows. A split only ever
    // replaces a patch with its visible children, so hitting kMaxPatches leaves
    // the coarsest acceptable coverage instead of punching holes in it.
    struct Entry
    {
        double pixels;
        TileMath::TileKey key;
    };

    const auto lessPixels = [](const Entry& a, const Entry& b) { return a.pixels < b.pixels; };
    std::priority_queue<Entry, std::vector<Entry>, decltype(lessPixels)> queue(lessPixels);

    QList<TileMath::TileKey> desired;

    const TileMath::TileKey root{0, 0, 0};
    if (visible.intersects(patchRect(root))) {
        queue.push(Entry{_projectedPixels(root, cameraGround, cameraHeight), root});
    }

    while (!queue.empty()) {
        const Entry entry = queue.top();
        if (entry.pixels <= kRefinePixelThreshold) {
            break;  // largest is small enough, so every remaining patch is too
        }
        if (entry.key.zoom >= TileMath::kMaxZoom) {
            queue.pop();
            desired.append(entry.key);  // cannot refine further; keep as-is
            continue;
        }

        QVarLengthArray<Entry, 4> children;
        for (int childY = 0; childY < 2; childY++) {
            for (int childX = 0; childX < 2; childX++) {
                const TileMath::TileKey childKey{(entry.key.x * 2) + childX, (entry.key.y * 2) + childY,
                                                 entry.key.zoom + 1};
                if (visible.intersects(patchRect(childKey))) {
                    children.append(Entry{_projectedPixels(childKey, cameraGround, cameraHeight), childKey});
                }
            }
        }
        const int totalAfterSplit = static_cast<int>(queue.size()) + desired.count() - 1 + children.count();
        if (totalAfterSplit > kMaxPatches) {
            break;  // budget exhausted: keep remaining patches coarse
        }
        queue.pop();
        for (const Entry& child : children) {
            queue.push(child);
        }
    }

    while (!queue.empty()) {
        desired.append(queue.top().key);
        queue.pop();
    }
    return desired;
}

void SurfaceModel::_heightsReady(int requestId, const QList<float>& heights)
{
    const auto it = _requestKeys.constFind(requestId);
    if (it == _requestKeys.constEnd()) {
        return;  // patch was dropped before delivery
    }
    const TileMath::TileKey key = it.value();
    _requestKeys.erase(it);

    const auto patchIt = _patches.find(key);
    if (patchIt == _patches.end()) {
        return;
    }
    patchIt.value().heights = heights;
    patchIt.value().ready = true;
    emit patchReady(key);
    _sweepRetiring();

    // Terrain taller than the last cull assumed may be visible below/behind
    // the camera: re-cull terrain-aware
    if (_maxTerrainZ() > (_culledTerrainZ + kRecullHeightMargin)) {
        update();
    }
}

void SurfaceModel::_sweepRetiring()
{
    // A retiring patch may go once every pending patch it covers for has its
    // heights (or it covers for none, e.g. it was panned out of the view)
    for (auto it = _patches.begin(); it != _patches.end();) {
        if (!it.value().retiring || _overlapsPendingPatch(it.key())) {
            ++it;
            continue;
        }
        const TileMath::TileKey key = it.key();
        it = _patches.erase(it);
        emit patchRemoved(key);
    }
}

bool SurfaceModel::_overlapsPendingPatch(const TileMath::TileKey& key) const
{
    const QRectF rect = patchRect(key);
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        // Touching edges do not intersect, so only true overlaps (the
        // quadtree ancestor/descendant replacements) count. Desired degraded
        // patches still need cover: their flat fallback is worse than the
        // cover. Retiring patches never do (they are on their way out) -
        // otherwise a retiring degraded patch would retain itself forever.
        if (it.value().retiring) {
            continue;
        }
        if ((!it.value().ready || it.value().degraded) && rect.intersects(patchRect(it.key()))) {
            return true;
        }
    }
    return false;
}

void SurfaceModel::_heightsFailed(int requestId)
{
    const auto it = _requestKeys.constFind(requestId);
    if (it == _requestKeys.constEnd()) {
        return;  // patch was dropped before delivery
    }
    const auto patchIt = _patches.find(it.value());
    if ((patchIt != _patches.end()) && !patchIt.value().ready && (patchIt.value().retriesLeft > 0)) {
        // Transient failures (elevation tile fetch timeouts) self-heal: keep the
        // patch pending and re-request after the backoff delay
        patchIt.value().retriesLeft--;
        patchIt.value().requestId = 0;
        const TileMath::TileKey key = patchIt.key();
        _requestKeys.erase(it);
        QTimer::singleShot(_heightRetryDelayMs, this, [this, key] { _retryHeights(key); });
        return;
    }
    // Retries exhausted: degrade to flat ground; a later cull/re-add starts
    // fresh. Degraded patches keep any retiring cover (see _overlapsPendingPatch),
    // so a real-height predecessor is not replaced by the flat fallback.
    if (patchIt != _patches.end()) {
        patchIt.value().degraded = true;
    }
    _heightsReady(requestId, QList<float>());
}

void SurfaceModel::_retryHeights(const TileMath::TileKey& key)
{
    const auto it = _patches.find(key);
    if ((it == _patches.end()) || it.value().ready || (it.value().requestId != 0)) {
        return;  // dropped, delivered, or re-requested meanwhile
    }
    it.value().requestId = _heightSource->requestPatchHeights(key, kGridSize);
    _requestKeys.insert(it.value().requestId, key);
}
