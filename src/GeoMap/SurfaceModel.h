/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtCore/QSet>
#include <QtCore/QVarLengthArray>

#include <optional>

#include "TileMath.h"

class GeoMapCamera;
class HeightField;
class HeightSource;

/// Maintains the active set of surface mesh patches for the current camera view.
///
/// Patches follow the slippy-tile quadtree. On every camera change the model
/// refines the quadtree by screen-space error (a patch subdivides while its
/// projected size exceeds the refinement threshold), culls against the visible
/// ground region, and diffs the result against the current set.
///
/// Patches are views of the continuous HeightField: a new patch meshes
/// immediately from the field's best estimate (never a flat placeholder with
/// real neighbors), tile coverage is requested from the height source, and
/// when data arrives the field's regionChanged re-meshes exactly the patches
/// touching the changed region. A patch being replaced across LOD changes may
/// only be removed once the replacements covering it are resident, so LOD
/// churn and panning never open holes in the surface.
///
/// The visible ground region is estimated by sampling screen points through the
/// camera and capping horizon misses at a distance-scaled range, so far-horizon
/// coverage at high tilt is approximate (coarse patches there are acceptable).
class SurfaceModel : public QObject
{
    Q_OBJECT

public:
    SurfaceModel(GeoMapCamera* camera, HeightSource* heightSource, HeightField* field, QObject* parent = nullptr);

    static constexpr int kGridSize = 16;  ///< mesh cells per patch edge
    /// Budget for the desired patch set; refinement stays coarser rather than exceed
    /// it. The resident set can transiently exceed it while retiring patches from the
    /// previous LOD generation cover for pending replacements (worst case ~2x).
    static constexpr int kMaxPatches = 256;
    /// Churn cap: each added patch synchronously creates a render delegate
    /// (geometry + texture, ~0.75 ms on desktop), so one update pass adds at
    /// most this many; the remainder streams over follow-up scheduled passes.
    /// Budgets size the worst-case pass, not throughput: saturated passes run
    /// back-to-back, so ops/sec is budget/passCost and stays constant.
    static constexpr int kMaxPatchAddsPerUpdate = 2;
    /// Removal counterpart: delegate destruction is also synchronous, and
    /// high-tilt pose jumps invalidate hundreds of patches at once (400+ ms
    /// single passes in capture data when removals were unbounded). Higher
    /// than the add cap so drains outpace fills under sustained churn.
    static constexpr int kMaxPatchRemovalsPerUpdate = 4;
    static constexpr double kRefinePixelThreshold = 384.0;  ///< subdivide above this projected size
    static constexpr double kMaxRangeMultiplier = 40.0;     ///< visible-range cap in camera distances
    static constexpr int kVisibleSampleGrid = 5;            ///< NxN screen samples for visible-region estimation
    /// Re-cull when resident terrain grows taller than the last cull assumed by
    /// more than this (scene units); avoids update churn on every patch delivery
    static constexpr double kRecullHeightMargin = 25.0;

    struct Patch
    {
        TileMath::TileKey key;
        QList<float> heights;  ///< (kGridSize+1)^2 field samples, row-major from the NW corner
        /// Always true: patches mesh immediately from the field's estimate.
        /// Kept until the downstream covered/ready plumbing is removed (step 8).
        bool ready = true;
        bool covered = false;  ///< always false (see ready)
    };

    /// Recompute the active patch set from the current camera state. Called
    /// automatically on camera changes; safe to call directly (idempotent).
    /// One call adds at most kMaxPatchAddsPerUpdate patches; leftovers are
    /// completed by self-scheduled follow-up passes (see updateSettled).
    void update();

    /// False while a scheduled update pass or deferred patch churn is outstanding
    bool updateSettled() const { return !_updatePending && !_addsDeferred && !_removalsDeferred; }

    QList<Patch> patches() const;

    /// Single-patch lookup; std::nullopt when the key is not resident
    std::optional<Patch> patch(const TileMath::TileKey& key) const;

    /// How many LOD levels coarser the resident neighbor across each edge
    /// renders, as {north, south, west, east} for PatchGeometry stitching.
    /// 0 = same/finer/no neighbor (unconstrained).
    QList<int> edgeLodDeltas(const TileMath::TileKey& key) const;

    /// update() call/duration counters for the perf overlay
    struct UpdateStats
    {
        int updates = 0;
        qint64 totalUs = 0;
        qint64 maxUs = 0;
        qint64 addTotalUs = 0;  ///< time inside the patch-add loop (delegate creation share)
        qint64 addMaxUs = 0;
    };

    /// Returns the counters accumulated since the previous call and resets them
    UpdateStats takeUpdateStats();

    /// Estimated visible ground region in world meters (see class comment)
    QRectF visibleGroundRect() const { return _visibleGroundRect(_maxTerrainZ()); }

    int patchCount() const { return _patches.count(); }

    /// Always 0: patches mesh immediately from the field. Kept for the perf
    /// overlay bindings until the downstream plumbing is removed (step 8).
    int pendingCount() const { return 0; }

#ifdef QGC_UNITTEST_BUILD
    /// Test hook: runs capped update passes synchronously to completion.
    /// The caps guarantee every pass makes progress, so this terminates.
    void drainUpdates()
    {
        int guard = kMaxPatches;
        do {
            update();
        } while ((_addsDeferred || _removalsDeferred) && (--guard > 0));
    }
#endif

signals:
    void patchAdded(const TileMath::TileKey& key);
    /// The patch's mesh content changed (heights re-sampled): consumers must re-pull heights
    void patchMeshChanged(const TileMath::TileKey& key);
    /// A neighbor add/remove changed the patch's edge LOD deltas: consumers
    /// must re-pull edgeLodDeltas (heights are unchanged)
    void patchEdgeDeltasChanged(const TileMath::TileKey& key);
    void patchRemoved(const TileMath::TileKey& key);

private slots:
    void _fieldRegionChanged(const QRectF& worldRect);

private:
    struct PatchData
    {
        QList<float> heights;    ///< cached field samples; refreshed on add and regionChanged
        float maxHeight = 0.0f;  ///< cached vertex max so _maxTerrainZ is O(patches) not O(vertices)
    };

    double _projectedPixels(const TileMath::TileKey& key, const QPointF& cameraGround, double cameraHeight) const;
    QList<TileMath::TileKey> _desiredPatches(const QRectF& visible, const QPointF& cameraGround,
                                             double cameraHeight) const;
    QRectF _visibleGroundRect(double terrainZ) const;
    double _maxTerrainZ() const;
    int _edgeDelta(const TileMath::TileKey& key, int dx, int dy) const;
    void _scheduleUpdate();

    // update() phases (see update() for the pass structure)
    struct AddResult
    {
        int adds = 0;
        qint64 addUs = 0;
    };

    AddResult _addDesiredPatches(const QList<TileMath::TileKey>& desired, QVarLengthArray<QRectF, 8>& churnRects);
    int _removeStalePatches(const QList<TileMath::TileKey>& desired, const QSet<TileMath::TileKey>& desiredSet,
                            QVarLengthArray<QRectF, 8>& churnRects);
    void _notifyEdgeChurn(const QVarLengthArray<QRectF, 8>& churnRects);
    void _repinAncestors();

    GeoMapCamera* const _camera;
    HeightSource* const _heightSource;
    HeightField* const _field;
    QHash<TileMath::TileKey, PatchData> _patches;
    UpdateStats _updateStats;
    bool _updatePending = false;     ///< a coalesced update pass is queued on the event loop
    bool _addsDeferred = false;      ///< the last pass hit the add cap; a follow-up pass is queued
    bool _removalsDeferred = false;  ///< the last pass hit the removal cap; a follow-up pass is queued
    double _culledTerrainZ = 0.0;    ///< terrain-top height assumed by the last cull (scene units)
};
