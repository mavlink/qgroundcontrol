#include "SurfaceModel.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include <QtCore/QElapsedTimer>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QtMath>
#include <QtPositioning/QGeoCoordinate>
#include <queue>

#include "Fact.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "HeightField.h"
#include "HeightSource.h"
#include "PatchMesh.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(GeoMapSurfaceModelLog, "GeoMap.SurfaceModel")
QGC_LOGGING_CATEGORY(GeoMapSurfaceModelVerboseLog, "GeoMap.SurfaceModel.Verbose")
QGC_LOGGING_CATEGORY(GeoMapSurfaceModelCliffsLog, "GeoMap.SurfaceModel.Cliffs")

namespace {

QRectF patchRect(const TileMath::TileKey& key)
{
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const QPointF minCorner = TileMath::tileMinCorner(key);
    return QRectF(minCorner.x(), minCorner.y(), span, span);
}

/// Region contact including edge-only touch: patch edge vertices exactly on
/// the region boundary sample the changed data, but QRectF::intersects is
/// false for edge-only contact, so the patch rect is inflated first
bool patchTouchesRegion(const TileMath::TileKey& key, const QRectF& region)
{
    const QRectF rect = patchRect(key);
    const double margin = rect.width() * 1e-6;
    return rect.marginsAdded(QMarginsF(margin, margin, margin, margin)).intersects(region);
}

float maxHeightOf(const QList<float>& heights)
{
    float maxHeight = 0.0f;
    for (const float height : heights) {
        if (std::isfinite(height)) {
            maxHeight = std::max(maxHeight, height);
        }
    }
    return maxHeight;
}

/// Edge of a boundary vertex; the cliff metric never reports corners
QString boundaryLabel(int row, int col, int gridSize)
{
    const QChar edge = (row == 0)          ? QLatin1Char('N')
                       : (row == gridSize) ? QLatin1Char('S')
                       : (col == 0)        ? QLatin1Char('W')
                                           : QLatin1Char('E');
    return edge + QStringLiteral(" edge");
}

}  // namespace

SurfaceModel::SurfaceModel(GeoMapCamera* camera, HeightSource* heightSource, HeightField* field, QObject* parent)
    : QObject(parent), _camera(camera), _heightSource(heightSource), _field(field)
{
    qRegisterMetaType<TileMath::TileKey>();
    _cliffClock.start();

    connect(_field, &HeightField::regionChanged, this, &SurfaceModel::_fieldRegionChanged);
    // Resident patches pick up (or drop) monitor state now, not on their next data change
    connect(SettingsManager::instance()->flyViewSettings()->geoMapDebugUI(), &Fact::rawValueChanged, this, [this]() {
        for (auto it = _patches.begin(); it != _patches.end(); ++it) {
            _samplePatch(it.key(), it.value());
        }
    });

    // Coalesce: camera signals fire per input event (up to 120/s during a
    // gesture); one queued pass per event-loop iteration bounds the cost
    connect(_camera, &GeoMapCamera::centerChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::headingChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::tiltChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::distanceChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::centerElevationChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::viewportSizeChanged, this, &SurfaceModel::_scheduleUpdate);
    connect(_camera, &GeoMapCamera::fieldOfViewChanged, this, &SurfaceModel::_scheduleUpdate);
}

void SurfaceModel::_scheduleUpdate()
{
    if (_updatePending) {
        return;
    }
    _updatePending = true;
    QTimer::singleShot(0, this, [this] {
        if (!_updatePending) {
            return;  // absorbed by a direct update() call meanwhile
        }
        update();
    });
}

void SurfaceModel::update()
{
    // Any direct pass supersedes a queued coalesced one (its singleShot
    // becomes a no-op), so updateSettled() is truthful right after a call
    _updatePending = false;
    if (!_camera->isPositioned()) {
        return;  // the default pose is null island; building patches there fires doomed terrain fetches
    }
    if (_camera->viewportSize().isEmpty()) {
        return;  // keep the last valid set; transient 0-size viewports occur during layout
    }

    QElapsedTimer updateTimer;
    updateTimer.start();

    // Max terrain height scans every resident vertex: compute once per update
    const double terrainZ = _maxTerrainZ();
    const QRectF visible = _visibleGroundRect(terrainZ);
    _culledTerrainZ = terrainZ;
    const QPointF cameraGround = _camera->cameraGroundPosition();
    const double cameraHeight = _camera->distance() * std::cos(qDegreesToRadians(_camera->tilt()));

    const QList<TileMath::TileKey> desired = _desiredPatches(visible, cameraGround, cameraHeight);

    QSet<TileMath::TileKey> desiredSet(desired.cbegin(), desired.cend());

    QVarLengthArray<QRectF, 8> churnRects;  // added/removed extents: neighbors there re-stitch
    const AddResult added = _addDesiredPatches(desired, churnRects);
    const int removals = _removeStalePatches(desired, desiredSet, churnRects);
    _notifyEdgeChurn(churnRects);

    if ((added.adds > 0) || (removals > 0)) {
        _repinAncestors();
        emit surfaceHeightsChanged();
    }

    // The caps guarantee every deferred pass makes progress, so follow-ups
    // always converge
    if (_addsDeferred || _removalsDeferred) {
        _scheduleUpdate();
    }

    const qint64 elapsedUs = updateTimer.nsecsElapsed() / 1000;
    qCDebug(GeoMapSurfaceModelVerboseLog)
        << "update pass: desired" << desired.count() << "resident" << _patches.count() << "adds" << added.adds
        << "removals" << removals << "deferred adds" << _addsDeferred << "deferred removals" << _removalsDeferred
        << "elapsedUs" << elapsedUs << "addUs" << added.addUs;
    _updateStats.updates++;
    _updateStats.totalUs += elapsedUs;
    _updateStats.maxUs = std::max(_updateStats.maxUs, elapsedUs);
}

/// Add newly desired patches, capped per pass: each add synchronously builds
/// a render delegate downstream, and uncapped bursts (300+ adds/s while
/// zooming) blow the frame budget. A new patch meshes immediately from the
/// field's best estimate; tile coverage is requested so the estimate refines
/// when data arrives.
SurfaceModel::AddResult SurfaceModel::_addDesiredPatches(const QList<TileMath::TileKey>& desired,
                                                         QVarLengthArray<QRectF, 8>& churnRects)
{
    AddResult result;
    _addsDeferred = false;
    QElapsedTimer addTimer;
    addTimer.start();
    for (const TileMath::TileKey& key : desired) {
        if (_patches.contains(key)) {
            continue;
        }
        if (result.adds >= kMaxPatchAddsPerUpdate) {
            _addsDeferred = true;
            continue;
        }
        PatchData data;
        _samplePatch(key, data);
        _patches.insert(key, std::move(data));
        churnRects.append(patchRect(key));
        _heightSource->requestTile(key);
        emit patchAdded(key);
        result.adds++;
    }
    result.addUs = addTimer.nsecsElapsed() / 1000;
    _updateStats.addTotalUs += result.addUs;
    _updateStats.addMaxUs = std::max(_updateStats.addMaxUs, result.addUs);
    return result;
}

/// Drop patches no longer desired. Removals are capped like adds: each erase
/// synchronously destroys a render delegate, and pose jumps (high-tilt
/// orbits) can invalidate hundreds of patches at once. Patches whose
/// replacements are not resident yet stay for a follow-up pass.
int SurfaceModel::_removeStalePatches(const QList<TileMath::TileKey>& desired,
                                      const QSet<TileMath::TileKey>& desiredSet, QVarLengthArray<QRectF, 8>& churnRects)
{
    // Rects of desired patches still not resident after the capped adds: a
    // no-longer-desired patch overlapping one may not be removed yet, or the
    // surface would show a hole until the adds catch up. Computed after the
    // adds so a replaced patch drops in the same pass its last replacement
    // arrives, instead of both rendering (and z-fighting) one extra pass.
    QVarLengthArray<QRectF, 16> missingRects;
    for (const TileMath::TileKey& key : desired) {
        if (!_patches.contains(key)) {
            missingRects.append(patchRect(key));
        }
    }
    const auto overlapsMissing = [&missingRects](const TileMath::TileKey& key) {
        const QRectF rect = patchRect(key);
        for (const QRectF& missing : missingRects) {
            if (rect.intersects(missing)) {
                return true;
            }
        }
        return false;
    };

    int removals = 0;
    _removalsDeferred = false;
    for (auto it = _patches.begin(); it != _patches.end();) {
        if (desiredSet.contains(it.key())) {
            ++it;
            continue;
        }
        if ((removals >= kMaxPatchRemovalsPerUpdate) || overlapsMissing(it.key())) {
            _removalsDeferred = true;  // stays resident one more pass; follow-up finishes the cull
            ++it;
            continue;
        }
        const TileMath::TileKey removedKey = it.key();
        if (it.value().cliffSinceMs >= 0) {
            qCWarning(GeoMapSurfaceModelCliffsLog) << "cliff removed with patch" << removedKey << "after"
                                                   << (_cliffClock.elapsed() - it.value().cliffSinceMs) << "ms";
        }
        it = _patches.erase(it);
        churnRects.append(patchRect(removedKey));
        emit patchRemoved(removedKey);
        removals++;
    }
    return removals;
}

/// Added/removed patches change their neighbors' edge LOD deltas: notify
/// every resident patch touching a churned extent so it re-stitches
void SurfaceModel::_notifyEdgeChurn(const QVarLengthArray<QRectF, 8>& churnRects)
{
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        for (const QRectF& rect : churnRects) {
            if (patchTouchesRegion(it.key(), rect)) {
                emit patchEdgeDeltasChanged(it.key());
                break;
            }
        }
    }
}

/// Pin every resident patch's key, its 8 neighbors' keys, and all their
/// ancestor chains: boundary vertices resolve into neighbor cells and the
/// interior blends toward them, so an eviction anywhere there would silently
/// change a rendered mesh
void SurfaceModel::_repinAncestors()
{
    QSet<TileMath::TileKey> pinned;
    const auto pinChain = [&pinned](TileMath::TileKey key) {
        // A pinned key already has its whole chain pinned
        while (!pinned.contains(key)) {
            pinned.insert(key);
            if (key.zoom == TileMath::kMinZoom) {
                return;
            }
            key = TileMath::TileKey{key.x >> 1, key.y >> 1, key.zoom - 1};
        }
    };
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        const TileMath::TileKey key = it.key();
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                const TileMath::TileKey neighbor{key.x + dx, key.y + dy, key.zoom};
                if (TileMath::isValidKey(neighbor)) {
                    pinChain(neighbor);
                }
            }
        }
    }
    _field->setPinnedKeys(std::move(pinned));
}

// GCOVR_EXCL_START — perf-stats instrumentation for the debug overlay/capture
SurfaceModel::UpdateStats SurfaceModel::takeUpdateStats()
{
    const UpdateStats stats = _updateStats;
    _updateStats = UpdateStats{};
    return stats;
}

// GCOVR_EXCL_STOP

QList<SurfaceModel::Patch> SurfaceModel::patches() const
{
    QList<Patch> result;
    result.reserve(_patches.count());
    for (auto it = _patches.cbegin(); it != _patches.cend(); ++it) {
        result.append(Patch{it.key(), it.value().heights, true, false, it.value().edgeStep, it.value().edgeStepAt,
                            it.value().edgeCliff});
    }
    return result;
}

std::optional<SurfaceModel::Patch> SurfaceModel::patch(const TileMath::TileKey& key) const
{
    const auto it = _patches.constFind(key);
    if (it == _patches.cend()) {
        return std::nullopt;
    }
    return Patch{
        key, it.value().heights, true, false, it.value().edgeStep, it.value().edgeStepAt, it.value().edgeCliff};
}

std::optional<double> SurfaceModel::renderedHeightAt(const QPointF& world) const
{
    // Finest first: a patch retiring behind resident replacements still overlaps them
    for (int zoom = TileMath::kMaxZoom; zoom >= TileMath::kMinZoom; zoom--) {
        const TileMath::TileKey owner = TileMath::tileForWorld(world, zoom);
        const QRectF ownerRect = patchRect(owner);
        // A point on a tile boundary maps to the east/south tile, but the west/north one draws it too
        const double onEdge = ownerRect.width() * 1e-9;
        const int westSteps = ((world.x() - ownerRect.left()) <= onEdge) ? 1 : 0;
        const int northSteps = ((ownerRect.bottom() - world.y()) <= onEdge) ? 1 : 0;
        TileMath::TileKey key = owner;
        auto it = _patches.cend();
        for (int dy = 0; (dy <= northSteps) && (it == _patches.cend()); dy++) {
            for (int dx = 0; (dx <= westSteps) && (it == _patches.cend()); dx++) {
                key = TileMath::TileKey{owner.x - dx, owner.y - dy, zoom};
                it = TileMath::isValidKey(key) ? _patches.constFind(key) : _patches.cend();
            }
        }
        if (it == _patches.cend()) {
            continue;
        }
        const QRectF rect = patchRect(key);
        const double cell = rect.width() / kGridSize;
        // Row 0 is the north (max y) edge
        const double col = (world.x() - rect.left()) / cell;
        const double row = ((rect.top() + rect.height()) - world.y()) / cell;
        // Stitching moves only edge vertices: interior cells skip the neighbor lookups
        PatchMesh::LodDeltas deltas{};
        if (row < 1.0) {
            deltas[0] = _edgeDelta(key, 0, -1);
        }
        if (row >= (kGridSize - 1)) {
            deltas[1] = _edgeDelta(key, 0, 1);
        }
        if (col < 1.0) {
            deltas[2] = _edgeDelta(key, -1, 0);
        }
        if (col >= (kGridSize - 1)) {
            deltas[3] = _edgeDelta(key, 1, 0);
        }
        return PatchMesh::surfaceHeight(it->heights, kGridSize, deltas, row, col);
    }
    return std::nullopt;
}

QList<int> SurfaceModel::edgeLodDeltas(const TileMath::TileKey& key) const
{
    // {N,S,W,E} neighbor offsets; slippy y grows south, so north is y-1
    static constexpr int kOffsets[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    QList<int> deltas;
    deltas.reserve(4);
    for (const auto& offset : kOffsets) {
        deltas.append(_edgeDelta(key, offset[0], offset[1]));
    }
    return deltas;
}

int SurfaceModel::_edgeDelta(const TileMath::TileKey& key, int dx, int dy) const
{
    const TileMath::TileKey neighbor{key.x + dx, key.y + dy, key.zoom};
    if (!TileMath::isValidKey(neighbor) || _patches.contains(neighbor)) {
        return 0;  // world edge or same-zoom neighbor: unconstrained
    }
    // First resident ancestor of the missing same-zoom neighbor is the coarse
    // patch rendering across this edge; finer neighbors constrain themselves
    TileMath::TileKey ancestor = neighbor;
    while (ancestor.zoom > TileMath::kMinZoom) {
        ancestor = TileMath::TileKey{ancestor.x >> 1, ancestor.y >> 1, ancestor.zoom - 1};
        if (_patches.contains(ancestor)) {
            const int delta = key.zoom - ancestor.zoom;
            // No coincident vertices beyond this: leave unconstrained (skirts cover)
            return ((kGridSize % (1 << delta)) == 0) ? delta : 0;
        }
    }
    return 0;
}

QRectF SurfaceModel::_visibleGroundRect(double terrainZ) const
{
    const QSizeF viewport = _camera->viewportSize();
    const double maxRange = _camera->distance() * kMaxRangeMultiplier;

    // Terrain-aware near boundary: rays that hit the ground plane far ahead
    // cross the terrain-top plane much closer to (or behind) the camera, so
    // tall terrain there is visible even though its flat-ground point is not
    const double eyeZ =
        _camera->centerElevation() + (_camera->distance() * std::cos(qDegreesToRadians(_camera->tilt())));
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
        maxHeight = std::max(maxHeight, data.maxHeight);
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
        (2.0 * distance * std::tan(qDegreesToRadians(_camera->verticalFieldOfView()) / 2.0)) / _camera->viewportSize().height();
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

void SurfaceModel::_fieldRegionChanged(const QRectF& worldRect)
{
    // Re-mesh exactly the patches touching the changed region: their field
    // samples (and skirt/normal data downstream) may have changed. Patch edge
    // vertices exactly on the region boundary sample the new data too, hence
    // the inflated-rect contact test.
    int remeshed = 0;
    for (auto it = _patches.begin(); it != _patches.end(); ++it) {
        if (!patchTouchesRegion(it.key(), worldRect)) {
            continue;
        }
        _samplePatch(it.key(), it.value());
        emit patchMeshChanged(it.key());
        remeshed++;
    }
    qCDebug(GeoMapSurfaceModelVerboseLog)
        << "regionChanged" << worldRect << "re-meshed" << remeshed << "of" << _patches.count() << "patches";
    // Emitted even with nothing re-meshed: heights beyond the patches come from the field
    emit surfaceHeightsChanged();

    // Terrain taller than the last cull assumed may be visible below/behind
    // the camera: re-cull terrain-aware
    if (_maxTerrainZ() > (_culledTerrainZ + kRecullHeightMargin)) {
        _scheduleUpdate();
    }
}

void SurfaceModel::_samplePatch(const TileMath::TileKey& key, PatchData& data)
{
    const bool monitorCliffs = SettingsManager::instance()->flyViewSettings()->geoMapDebugUI()->rawValue().toBool();
    PatchSampler::EdgeStep edgeStep;
    data.heights = _field->samplePatch(key, kGridSize, monitorCliffs ? &edgeStep : nullptr);
    data.maxHeight = maxHeightOf(data.heights);
    data.edgeStep = edgeStep.step;
    data.edgeCliff = false;
    if (!monitorCliffs) {
        data.cliffSinceMs = -1;
        return;
    }
    const QRectF rect = patchRect(key);
    const double cell = rect.width() / kGridSize;
    data.edgeStepAt =
        QPointF(rect.left() + (edgeStep.col * cell), (rect.top() + rect.height()) - (edgeStep.row * cell));
    const QGeoCoordinate stepAt = TileMath::worldToGeo(data.edgeStepAt);
    // Mercator meters overstate ground distance by 1/cos(latitude)
    const double groundCell = cell * std::cos(qDegreesToRadians(stepAt.latitude()));

    const bool cliff = edgeStep.step >= (kCliffLogSlope * groundCell);
    data.edgeCliff = cliff;
    if (cliff && (data.cliffSinceMs < 0)) {
        data.cliffSinceMs = _cliffClock.elapsed();
        const QString interiorSource = (edgeStep.ownZoom < 0) ? QStringLiteral("no elevation data (flat 0 m)")
                                                              : QStringLiteral("z%1 tile").arg(edgeStep.ownZoom);
        qCWarning(GeoMapSurfaceModelCliffsLog).noquote()
            << "cliff appeared: patch" << key << boundaryLabel(edgeStep.row, edgeStep.col, kGridSize) << "step"
            << edgeStep.step << "m over a" << qRound(groundCell) << "m cell"
            << "at" << stepAt << "| patch interior:" << interiorSource
            << QStringLiteral("| edge vertex: z%1 tile").arg(edgeStep.boundaryZoom);
    } else if (!cliff && (data.cliffSinceMs >= 0)) {
        qCWarning(GeoMapSurfaceModelCliffsLog)
            << "cliff cleared: patch" << key << "after" << (_cliffClock.elapsed() - data.cliffSinceMs) << "ms";
        data.cliffSinceMs = -1;
    }
}
