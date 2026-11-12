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
#include <QtCore/QSet>

#include "ElevationTilePyramid.h"
#include "HeightSource.h"

class DecodedTileFetcher;
class QImage;
class QNetworkAccessManager;

/// Fetcher over the AWS Open Data Terrain Tiles (terrarium encoding): real
/// elevations at every patch zoom from one slippy PNG tile per patch, cache-first
/// through QGC's shared tile database with a direct network fallback on miss
/// (fetched tiles are stored back).
///
/// requestTile delivers whole decoded tiles into an attached HeightField, which
/// notifies consumers via regionChanged. A failed fetch inserts nothing: the
/// field keeps serving its current best estimate, and tileFailed reports it.
/// A tile that the provider does not have inserts nothing either, and is not
/// fetched again: that is no failure.
///
/// A tile at z/x/y maps to the terrarium tile at the same z/x/y, so fetch cost
/// is constant across the LOD range — no flat floor or blend band like the fixed
/// resolution Copernicus pipeline needed. Above the dataset's top zoom the z15
/// ancestor tile is fetched instead.
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
    /// data next to no data at all (a cliff to 0 m). An anchor that failed, or that
    /// the provider does not have, stops blocking.
    static constexpr int kAnchorZoom = 10;

    /// Ensures the attached field holds this tile (clamped to the z15 ancestor
    /// above kMaxTileZoom), plus the kAnchorZoom ancestors of it and its neighbors. Returns false for a request
    /// it rejects: no field is attached, or the key is invalid. True when the tile is already held, already in
    /// flight, known to be missing at the provider, or a fetch was started: a fetch that fails is reported by
    /// tileFailed.
    bool requestTile(const TileMath::TileKey& key) override;

    /// Tile key submitted by the most recent fetch (test observability for the
    /// deep-zoom ancestor path)
    TileMath::TileKey lastFetchKey() const { return _lastFetchKey; }

private:
    struct HeldTile
    {
        ElevationTilePyramid::Grid grid;
        QSet<TileMath::TileKey> anchors;  ///< anchors still in flight
    };

    void _tileFetched(const TileMath::TileKey& fetchKey, const QImage& image);
    /// Ends the wait for a tile that does not come, and inserts the tiles held back for it
    void _tileNotFetched(const TileMath::TileKey& fetchKey);
    void _insertIntoField(const TileMath::TileKey& fetchKey, ElevationTilePyramid::Grid grid);
    /// Drops \a anchor from every held tile's wait set and inserts the tiles
    /// left waiting on nothing; returns how many were released
    int _releaseHeld(const TileMath::TileKey& anchor);

    /// Bounds _missingTiles; at the bound it is emptied, and those tiles are fetched once more when asked for
    static constexpr int kMaxMissingTiles = 4096;

    DecodedTileFetcher* const _fetcher;
    QSet<TileMath::TileKey> _fieldRequests;    ///< tiles awaiting delivery into the field
    QHash<TileMath::TileKey, HeldTile> _held;  ///< delivered finer tiles waiting on their anchors
    QSet<TileMath::TileKey> _missingTiles;     ///< tiles the provider does not have
    TileMath::TileKey _lastFetchKey;
};
