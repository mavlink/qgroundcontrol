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
#include <QtCore/QRectF>
#include <QtCore/QSet>

#include <utility>

#include "TileMath.h"

/// In-memory working set of decoded elevation tiles, keyed by slippy tile.
///
/// This is the synchronous sampling layer of the continuous-drape design: the
/// persistent store remains QGC's shared tile cache database (encoded tiles,
/// async); this pyramid retains what the fetch path has already decoded so a
/// height estimate is answerable immediately at mesh time. Lookup serves a
/// query tile from itself or its nearest stored ancestor with the sub-window
/// to sample — never a descendant — so coverage is continuous wherever any
/// ancestor data exists.
///
/// Not thread-safe: confine to one thread or synchronize externally. This
/// includes the const lookup methods — they mutate recency/instrumentation
/// state, so even concurrent reads race.
/// Bounded working set: least-recently-used tiles are evicted past kMaxTiles.
class ElevationTilePyramid
{
public:
    /// Working-set cap: least-recently-used tiles are evicted at insert time
    /// (256x256 float tiles ~256KB each, so the cap bounds memory at ~32MB)
    static constexpr int kMaxTiles = 128;

    /// Decoded elevation samples for one tile, row-major from the NW corner
    struct Grid
    {
        int width = 0;         ///< samples per row
        int height = 0;        ///< rows
        QList<float> heights;  ///< meters

        bool isValid() const { return (width > 0) && (height > 0) && (heights.size() == qsizetype(width) * height); }
    };

    /// Where to sample a query tile: the stored tile and the query's extent
    /// within it (unit UV from the NW corner)
    struct View
    {
        const Grid* grid = nullptr;  ///< null when nothing stored covers the query
        TileMath::TileKey key;       ///< stored tile the view samples
        QRectF subWindow;

        bool isValid() const { return grid != nullptr; }
    };

    /// Stores \a grid for \a key, replacing any previous tile. Invalid grids
    /// are rejected (returns false). When the insert evicts a tile, its key is
    /// written to \a evictedKey (left untouched otherwise).
    bool insertTile(const TileMath::TileKey& key, Grid grid, TileMath::TileKey* evictedKey = nullptr);

    bool hasTile(const TileMath::TileKey& key) const { return _tiles.contains(key); }

    /// Finest stored tile covering \a key: the tile itself when present, else
    /// the nearest ancestor. View pointers stay valid until the next insert.
    View bestTileFor(const TileMath::TileKey& key) const;

    int tileCount() const { return static_cast<int>(_tiles.count()); }

    /// Tiles that must not be evicted (they back rendered patches or resolve
    /// them as ancestors). kMaxTiles becomes a soft cap: when every resident
    /// tile is pinned, inserts grow past it rather than break a rendered mesh.
    void setPinnedKeys(QSet<TileMath::TileKey> keys) { _pinnedKeys = std::move(keys); }

    /// True when any stored tile lies strictly deeper within \a key's extent
    /// (i.e. a lookup inside \a key could resolve finer than \a key itself)
    bool hasDescendant(const TileMath::TileKey& key) const { return _descendantCounts.value(key, 0) > 0; }

    /// Perf instrumentation: number of bestTileFor resolutions performed
    /// (test hook, not API — semantics track the resolution strategy)
    qint64 lookupCountForTest() const { return _lookupCount; }

private:
    TileMath::TileKey _evictLeastRecentlyUsed();

    QHash<TileMath::TileKey, Grid> _tiles;
    QHash<TileMath::TileKey, int> _descendantCounts;  ///< stored tiles strictly below each key
    QSet<TileMath::TileKey> _pinnedKeys;
    mutable QHash<TileMath::TileKey, qint64> _lastUsed;
    mutable qint64 _useTick = 0;
    mutable qint64 _lookupCount = 0;
};
