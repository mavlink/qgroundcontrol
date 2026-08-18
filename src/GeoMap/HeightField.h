/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtQmlIntegration/QtQmlIntegration>

#include "ElevationTilePyramid.h"
#include "TileMath.h"

/// The one continuous terrain heightfield of the drape design: best-estimate
/// height everywhere, by construction — real data where a tile is stored,
/// ancestor-interpolated estimate where only coarser data exists, zero where
/// nothing is known. There is no "missing" region, only coarser-estimate
/// regions, so any two callers asking for the same world position always get
/// the same answer regardless of which patch they mesh.
///
/// Heights are sampled bilinearly between grid sample centers (pixel-center
/// convention, clamped at tile edges), matching the terrarium tile layout.
///
/// Not thread-safe: confine to one thread or synchronize externally. This
/// includes the const sampling methods — they mutate an internal memo cache,
/// so even concurrent reads race.
class HeightField : public QObject
{
    Q_OBJECT
    // Passed through QML as an opaque pointer (PatchGeometry.heightField)
    QML_ANONYMOUS

public:
    explicit HeightField(QObject* parent = nullptr);

    /// Sanity cap on patch density: rejects absurd sizes before allocation
    static constexpr int kMaxGridSize = 4096;

    /// Stores a decoded tile in the backing pyramid; invalid keys/grids
    /// rejected. Emits regionChanged for the tile's world extent on success,
    /// and additionally for the extent of any tile the insert evicted —
    /// eviction changes the answer there too.
    bool insertTile(const TileMath::TileKey& key, ElevationTilePyramid::Grid grid);

    /// Tiles that must not be evicted because they back rendered patches
    /// (or resolve them as ancestors); see ElevationTilePyramid::setPinnedKeys
    void setPinnedKeys(QSet<TileMath::TileKey> keys) { _pyramid.setPinnedKeys(std::move(keys)); }

    /// Best-estimate height (meters) at a mercator world position; 0.0 where
    /// no stored tile covers the position
    double heightAt(const QPointF& world) const;

    /// The (gridSize+1)^2 vertex heights of a patch, row-major from the NW
    /// corner. Empty for invalid keys or gridSize outside [1, kMaxGridSize].
    /// gridSize must be a power of two: vertex UVs are then exact dyadic
    /// doubles, which the bit-identity guarantee below relies on.
    /// Boundary vertices resolve by canonical world position rather than the
    /// patch's own backing view, so coincident vertices of neighboring
    /// patches sample bit-identical heights even across different backing
    /// tiles — patch edges never crack where data exists.
    QList<float> samplePatch(const TileMath::TileKey& key, int gridSize) const;

    int tileCount() const { return _pyramid.tileCount(); }

    /// True when the backing pyramid holds this exact tile
    bool hasTile(const TileMath::TileKey& key) const { return _pyramid.hasTile(key); }

    /// Stored tile that backs samples for \a key (the tile itself when
    /// present, else its nearest stored ancestor); invalid when nothing covers it
    TileMath::TileKey backingKeyFor(const TileMath::TileKey& key) const
    {
        const ElevationTilePyramid::View view = _pyramid.bestTileFor(key);
        return view.isValid() ? view.key : TileMath::TileKey{0, 0, -1};
    }

    /// Perf instrumentation: pyramid resolutions performed so far (memoized
    /// sampling keeps this far below one per sampled vertex). Test hook, not
    /// API — semantics track the resolution strategy.
    qint64 lookupCountForTest() const { return _pyramid.lookupCountForTest(); }

signals:
    /// Best-estimate heights changed within this rect (TileMath world meters,
    /// y north; min-corner + positive spans — don't use top()/bottom()).
    /// The rect is closed: patch edge vertices exactly on its boundary sample
    /// the new data, but QRectF::intersects() is false for edge-only contact,
    /// so inflate the patch rect (e.g. marginsAdded) before testing
    void regionChanged(const QRectF& worldRect);

private:
    ElevationTilePyramid _pyramid;

    // Memoized last resolved view: adjacent sample positions almost always
    // resolve to the same stored tile, so heightAt reuses it when the query
    // key resolves to it (same arithmetic as the full lookup). Only populated
    // when the tile has no stored descendant (nothing finer could override
    // it), and invalidated on every insert (the view's grid pointer is only
    // valid until then).
    mutable ElevationTilePyramid::View _memoView;
    mutable double _memoMinX = 0.0;
    mutable double _memoMaxY = 0.0;
    mutable double _memoSpan = 0.0;
};
