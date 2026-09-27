/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QRectF>
#include <QtCore/QSet>

#include "ElevationTilePyramid.h"
#include "HeightSource.h"

class HeightField;
class QImage;
class QNetworkAccessManager;
class QNetworkReply;

/// Fetcher over the AWS Open Data Terrain Tiles (terrarium encoding): real
/// elevations at every patch zoom from one slippy PNG tile per patch, cache-first
/// through QGC's shared tile database with a direct network fallback on miss
/// (fetched tiles are stored back).
///
/// requestTile delivers whole decoded tiles into an attached HeightField, which
/// notifies consumers via regionChanged. A failed fetch inserts nothing: the
/// field keeps serving its current best estimate.
///
/// A tile at z/x/y maps to the terrarium tile at the same z/x/y, so fetch cost
/// is constant across the LOD range — no flat floor or blend band like the fixed
/// resolution Copernicus pipeline needed. Above the dataset's top zoom the z15
/// ancestor tile is fetched instead.
///
/// The legacy per-patch HeightSource API (bilinear vertex-grid sampling with
/// edge clamp) remains until SurfaceModel switches to sampling the field.
class TerrariumTileFetcher : public HeightSource
{
    Q_OBJECT

public:
    /// \a networkManager overrides the internally created one (test injection seam)
    explicit TerrariumTileFetcher(QObject* parent = nullptr, QNetworkAccessManager* networkManager = nullptr);

    /// Highest zoom the terrarium dataset serves (deeper patches sample the ancestor)
    static constexpr int kMaxTileZoom = 15;

    /// Coarse ancestor zoom fetched alongside every finer tile, for the tile and
    /// its neighbors; finer tiles wait for all of them so no region ever has fine
    /// data next to no data at all (a cliff to 0 m). A failed anchor stops blocking.
    static constexpr int kAnchorZoom = 10;

    /// Largest supported patch grid (matches PatchGeometry::kMaxGridSize)
    static constexpr int kMaxGridSize = 256;

    int requestPatchHeights(const TileMath::TileKey& key, int gridSize) final;
    void cancelRequest(int requestId) final;

    /// Ensures the attached field holds this tile (clamped to the z15 ancestor
    /// above kMaxTileZoom), plus the kAnchorZoom ancestors of it and its neighbors. Returns false when no
    /// field is attached, the key is invalid, or the fetch could not start; true
    /// when the tile is already held, already in flight, or a fetch was started.
    bool requestTile(const TileMath::TileKey& key) override;

    int pendingCount() const { return _pending.count(); }

    /// Tile key submitted by the most recent fetch (test observability for the
    /// deep-zoom ancestor path)
    TileMath::TileKey lastFetchKey() const { return _lastFetchKey; }

private:
    struct PendingRequest
    {
        TileMath::TileKey fetchKey;  ///< tile actually fetched (ancestor for deep zooms)
        QRectF subWindow;            ///< patch extent within the fetched tile, unit UV from the NW corner
        int gridSize = 0;
    };

    struct HeldTile
    {
        ElevationTilePyramid::Grid grid;
        QSet<TileMath::TileKey> anchors;  ///< anchors still in flight
    };

    void _insertIntoField(const TileMath::TileKey& fetchKey, ElevationTilePyramid::Grid grid);
    /// Drops \a anchor from every held tile's wait set and inserts the tiles
    /// left waiting on nothing; returns how many were released
    int _releaseHeld(const TileMath::TileKey& anchor);
    void _failAsync(int requestId);
    bool _fetchInFlight(const TileMath::TileKey& fetchKey) const;
    bool _startFetch(const TileMath::TileKey& fetchKey);
    void _fetchFromNetwork(const TileMath::TileKey& fetchKey);
    void _deliverAll(const TileMath::TileKey& fetchKey, const QImage& image);
    void _failAll(const TileMath::TileKey& fetchKey, const QString& reason);
    void _deliver(int requestId, const QImage& image);
    void _finishFailed(int requestId);
    bool _shouldWarnFailure();

    static constexpr int kFailureWarnIntervalMs = 10000;  ///< throttle for fetch-failure warnings

    QNetworkAccessManager* _networkManager = nullptr;
    const int _mapId;
    QHash<int, PendingRequest> _pending;
    QHash<TileMath::TileKey, QList<int>> _waiters;            ///< request ids sharing the in-flight fetch of a tile
    QSet<TileMath::TileKey> _fieldRequests;                   ///< tiles awaiting delivery into the field
    QHash<TileMath::TileKey, HeldTile> _held;                 ///< delivered finer tiles waiting on their anchors
    QHash<TileMath::TileKey, QNetworkReply*> _activeReplies;  ///< in-flight network fetches by tile
    TileMath::TileKey _lastFetchKey;
    QElapsedTimer _failureWarnTimer;                          ///< restarted on each emitted failure warning
};
