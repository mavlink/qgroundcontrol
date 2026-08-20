/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HeightField.h"

#include <utility>

#include "PatchSampler.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GeoMapHeightFieldLog, "GeoMap.HeightField")
QGC_LOGGING_CATEGORY(GeoMapHeightFieldVerboseLog, "GeoMap.HeightField.Verbose")

HeightField::HeightField(QObject* parent) : QObject(parent) {}

bool HeightField::insertTile(const TileMath::TileKey& key, ElevationTilePyramid::Grid grid)
{
    // Capture before the move: on rejection the grid has been consumed
    const int gridWidth = grid.width;
    const int gridHeight = grid.height;
    TileMath::TileKey evicted{0, 0, -1};
    if (!_pyramid.insertTile(key, std::move(grid), &evicted)) {
        qCWarning(GeoMapHeightFieldLog) << "insertTile rejected: key" << key << "grid" << gridWidth << "x"
                                        << gridHeight;
        return false;
    }
    _memoView = ElevationTilePyramid::View{};  // grid pointers die on insert
    qCDebug(GeoMapHeightFieldVerboseLog) << "inserted tile" << key;

    const auto tileExtent = [](const TileMath::TileKey& k) {
        const QPointF corner = TileMath::tileMinCorner(k);
        const double span = TileMath::tileSpanAtZoom(k.zoom);
        return QRectF(corner.x(), corner.y(), span, span);
    };
    if (TileMath::isValidKey(evicted)) {
        qCDebug(GeoMapHeightFieldVerboseLog) << "evicted tile" << evicted;
        emit regionChanged(tileExtent(evicted));
    }
    emit regionChanged(tileExtent(key));
    return true;
}

double HeightField::heightAt(const QPointF& world) const
{
    const TileMath::TileKey query = TileMath::tileForWorld(world, TileMath::kMaxZoom);

    // Memoized fast path: the last resolved tile answers when the position
    // resolves to it (the memo is only populated when no stored descendant
    // could override it, and every insert invalidates it). The hit test uses
    // the same key arithmetic as the full lookup — a world-coordinate bounds
    // check can disagree with tileForWorld by an ulp exactly on a tile
    // boundary, silently answering with the wrong neighbor's data there.
    if (_memoView.isValid()) {
        const int shift = TileMath::kMaxZoom - _memoView.key.zoom;
        if (((query.x >> shift) == _memoView.key.x) && ((query.y >> shift) == _memoView.key.y)) {
            const double u = (world.x() - _memoMinX) / _memoSpan;
            const double v = (_memoMaxY - world.y()) / _memoSpan;  // grid origin is the NW corner
            return PatchSampler::heightAtUV(*_memoView.grid, u, v);
        }
    }

    // The pyramid resolves the finest stored cover of the deepest-zoom query
    const ElevationTilePyramid::View view = _pyramid.bestTileFor(query);
    if (!view.isValid()) {
        return 0.0;
    }

    const QPointF corner = TileMath::tileMinCorner(view.key);
    const double span = TileMath::tileSpanAtZoom(view.key.zoom);
    if (!_pyramid.hasDescendant(view.key)) {
        // Nothing finer exists anywhere inside this tile, so it answers for
        // every position within its bounds until the next insert
        _memoView = view;
        _memoMinX = corner.x();
        _memoMaxY = corner.y() + span;
        _memoSpan = span;
    }

    const double u = (world.x() - corner.x()) / span;
    const double v = ((corner.y() + span) - world.y()) / span;  // grid origin is the NW corner
    return PatchSampler::heightAtUV(*view.grid, u, v);
}

QList<float> HeightField::samplePatch(const TileMath::TileKey& key, int gridSize) const
{
    if ((gridSize < 1) || (gridSize > kMaxGridSize) || !TileMath::isValidKey(key)) {
        qCWarning(GeoMapHeightFieldLog) << "samplePatch rejected: key" << key << "gridSize" << gridSize;
        return QList<float>();
    }

    // Interior vertices resolve the patch's backing view once, by the
    // patch's own key, and interpolate within that one grid. Boundary
    // vertices are shared with neighbor patches whose backing views can
    // differ (adjacent exact tiles, fine tile next to an ancestor-backed
    // neighbor), so they resolve canonically by position instead: the
    // deepest-zoom cells touching the vertex, tried east/south first, pick
    // the same stored tile no matter which patch asks. Positions are exact
    // dyadic values, so coincident vertices compute bit-identical UVs and
    // sample bit-identical heights: meshes never crack where data exists.
    return PatchSampler(_pyramid, key, gridSize).sample();
}
