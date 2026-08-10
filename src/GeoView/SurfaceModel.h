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
#include <optional>

#include "TileMath.h"

class GeoViewCamera;
class HeightSource;

/// Maintains the active set of surface mesh patches for the current camera view.
///
/// Patches follow the slippy-tile quadtree. On every camera change the model
/// refines the quadtree by screen-space error (a patch subdivides while its
/// projected size exceeds the refinement threshold), culls against the visible
/// ground region, and diffs the result against the current set: new patches get
/// height requests from the HeightSource, dropped pending patches get their
/// requests cancelled, and dropped ready patches retire - they keep rendering
/// until the patches replacing them have heights, so LOD churn and panning
/// never open holes in the surface.
///
/// The visible ground region is estimated by sampling screen points through the
/// camera and capping horizon misses at a distance-scaled range, so far-horizon
/// coverage at high tilt is approximate (coarse patches there are acceptable).
class SurfaceModel : public QObject
{
    Q_OBJECT

public:
    SurfaceModel(GeoViewCamera* camera, HeightSource* heightSource, QObject* parent = nullptr);

    static constexpr int kGridSize = 16;  ///< mesh cells per patch edge
    /// Budget for the desired patch set; refinement stays coarser rather than exceed
    /// it. The resident set can transiently exceed it while retiring patches from the
    /// previous LOD generation cover for pending replacements (worst case ~2x).
    static constexpr int kMaxPatches = 256;
    static constexpr double kRefinePixelThreshold = 384.0;  ///< subdivide above this projected size
    static constexpr double kMaxRangeMultiplier = 40.0;     ///< visible-range cap in camera distances
    static constexpr int kVisibleSampleGrid = 5;            ///< NxN screen samples for visible-region estimation
    static constexpr int kMaxHeightRetries = 3;             ///< re-requests before a patch degrades to flat
    /// Default retry delay: past TerrainTileManager's 5s failed-tile backoff so a
    /// retry refetches instead of short-circuiting on the cached failure
    static constexpr int kDefaultHeightRetryDelayMs = 6000;

    struct Patch
    {
        TileMath::TileKey key;
        QList<float> heights;  ///< (kGridSize+1)^2 entries once ready; empty = flat (pending or failed request)
        bool ready = false;
        bool covered = false;  ///< pending and overlapped by a ready patch: suppress rendering
    };

    /// Recompute the active patch set from the current camera state. Called
    /// automatically on camera changes; safe to call directly (idempotent).
    void update();

    QList<Patch> patches() const;

    /// Single-patch lookup; std::nullopt when the key is not resident
    std::optional<Patch> patch(const TileMath::TileKey& key) const;

    int patchCount() const { return _patches.count(); }

    int pendingCount() const;

#ifdef QGC_UNITTEST_BUILD
    /// Test hook: shortens the failed-request retry delay
    void setHeightRetryDelayMs(int ms) { _heightRetryDelayMs = ms; }
#endif

signals:
    void patchAdded(const TileMath::TileKey& key);
    void patchReady(const TileMath::TileKey& key);
    void patchRemoved(const TileMath::TileKey& key);

private slots:
    void _heightsReady(int requestId, const QList<float>& heights);
    void _heightsFailed(int requestId);

private:
    struct PatchData
    {
        int requestId = 0;  ///< 0 = no request in flight (awaiting retry or ready)
        QList<float> heights;
        bool ready = false;
        bool retiring = false;  ///< ready but replaced; renders until replacements are ready
        bool degraded = false;  ///< flat fallback after exhausted retries; keeps its retiring cover
        int retriesLeft = kMaxHeightRetries;
    };

    QRectF _visibleGroundRect() const;
    double _projectedPixels(const TileMath::TileKey& key, const QPointF& cameraGround, double cameraHeight) const;
    QList<TileMath::TileKey> _desiredPatches(const QRectF& visible, const QPointF& cameraGround,
                                             double cameraHeight) const;
    void _retryHeights(const TileMath::TileKey& key);
    void _sweepRetiring();
    bool _overlapsPendingPatch(const TileMath::TileKey& key) const;
    bool _isCovered(const TileMath::TileKey& key, const PatchData& data) const;

    GeoViewCamera* const _camera;
    HeightSource* const _heightSource;
    QHash<TileMath::TileKey, PatchData> _patches;
    QHash<int, TileMath::TileKey> _requestKeys;
    int _heightRetryDelayMs = kDefaultHeightRetryDelayMs;
};
