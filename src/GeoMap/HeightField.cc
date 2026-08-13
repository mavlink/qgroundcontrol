/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HeightField.h"

#include <QtCore/QVarLengthArray>
#include <QtCore/QtMinMax>

#include <cmath>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GeoMapHeightFieldLog, "GeoMap.HeightField")
QGC_LOGGING_CATEGORY(GeoMapHeightFieldVerboseLog, "GeoMap.HeightField.Verbose")

namespace {

/// Bilinear height at a unit-UV position within a grid (origin NW corner),
/// sample-center convention, clamped at grid edges.
/// Must stay numerically identical to TerrariumTileFetcher.cc's heightAtUV:
/// cross-source vertex identity depends on both samplers agreeing
double heightAtUV(const ElevationTilePyramid::Grid& grid, double u, double v)
{
    const int w = grid.width;
    const int h = grid.height;
    const double px = (u * w) - 0.5;
    const double py = (v * h) - 0.5;
    const int x0 = qBound(0, static_cast<int>(std::floor(px)), w - 1);
    const int y0 = qBound(0, static_cast<int>(std::floor(py)), h - 1);
    const int x1 = qMin(x0 + 1, w - 1);
    const int y1 = qMin(y0 + 1, h - 1);
    const double fx = qBound(0.0, px - x0, 1.0);
    const double fy = qBound(0.0, py - y0, 1.0);

    const auto at = [&grid](int x, int y) { return double(grid.heights[(qsizetype(y) * grid.width) + x]); };
    const double north = (at(x0, y0) * (1.0 - fx)) + (at(x1, y0) * fx);
    const double south = (at(x0, y1) * (1.0 - fx)) + (at(x1, y1) * fx);
    return (north * (1.0 - fy)) + (south * fy);
}

}  // namespace

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
            return heightAtUV(*_memoView.grid, u, v);
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
    return heightAtUV(*view.grid, u, v);
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
    const ElevationTilePyramid::View patchView = _pyramid.bestTileFor(key);

    // Height at the exact vertex position (n/gridSize, m/gridSize in tile
    // units at key.zoom) within a resolved view; ldexp rescales exactly, so
    // equal positions give equal UV bits regardless of the asking patch
    const auto viewHeight = [&key, gridSize](const ElevationTilePyramid::View& view, qint64 n, qint64 m) {
        const double u = (std::ldexp(double(n), view.key.zoom - key.zoom) / gridSize) - view.key.x;
        const double v = (std::ldexp(double(m), view.key.zoom - key.zoom) / gridSize) - view.key.y;
        return static_cast<float>(heightAtUV(*view.grid, u, v));
    };

    // Memo of resolved views per key.zoom tile: when a tile has no stored
    // descendant, every cell inside it resolves identically (chain below the
    // tile is empty, ancestors are shared), so one lookup answers the whole
    // edge run along it. An invalid view memoizes the same way — repeated
    // misses over uncovered neighbors stay O(1). A patch's boundary touches
    // at most 9 such tiles (own + 8 neighbors).
    struct TileMemo
    {
        TileMath::TileKey tile;
        ElevationTilePyramid::View view;
    };

    QVarLengthArray<TileMemo, 9> memos;

    const int shiftToMax = TileMath::kMaxZoom - key.zoom;
    const auto resolveCell = [&, this](qint64 cx, qint64 cy) {
        const TileMath::TileKey tile{int(cx >> shiftToMax), int(cy >> shiftToMax), key.zoom};
        for (const TileMemo& memo : memos) {
            if (memo.tile == tile) {
                return memo.view;
            }
        }
        const ElevationTilePyramid::View view =
            _pyramid.bestTileFor(TileMath::TileKey{int(cx), int(cy), TileMath::kMaxZoom});
        if (!_pyramid.hasDescendant(tile) && (memos.size() < memos.capacity())) {
            memos.append({tile, view});
        }
        return view;
    };

    const auto boundaryHeight = [&](qint64 n, qint64 m) -> float {
        // Cells touching the vertex on each axis, east/south side first; a
        // vertex not exactly on a cell boundary lies in a single cell
        const qint64 cellCount = qint64(1) << TileMath::kMaxZoom;
        const auto touchingCells = [&](qint64 s, qint64(&cells)[2]) {
            int count = 0;
            const qint64 cell = s / gridSize;
            if (cell < cellCount) {
                cells[count++] = cell;
            }
            if (((s % gridSize) == 0) && (cell > 0)) {
                cells[count++] = cell - 1;
            }
            return count;
        };
        qint64 xCells[2];
        qint64 yCells[2];
        const int xCount = touchingCells(n << shiftToMax, xCells);
        const int yCount = touchingCells(m << shiftToMax, yCells);

        // Fixed candidate order derived purely from the position: every
        // patch sharing this vertex walks the same cells and returns the
        // first resolvable view, so the height is canonical
        for (int yi = 0; yi < yCount; yi++) {
            for (int xi = 0; xi < xCount; xi++) {
                const ElevationTilePyramid::View view = resolveCell(xCells[xi], yCells[yi]);
                if (view.isValid()) {
                    return viewHeight(view, n, m);
                }
            }
        }
        return 0.0f;  // no stored data touches the vertex: every sharer agrees on zero
    };

    QList<float> heights;
    heights.reserve(qsizetype(gridSize + 1) * (gridSize + 1));
    for (int row = 0; row <= gridSize; row++) {
        const qint64 m = (qint64(key.y) * gridSize) + row;
        for (int col = 0; col <= gridSize; col++) {
            const qint64 n = (qint64(key.x) * gridSize) + col;
            if ((row == 0) || (row == gridSize) || (col == 0) || (col == gridSize)) {
                heights.append(boundaryHeight(n, m));
            } else {
                heights.append(patchView.isValid() ? viewHeight(patchView, n, m) : 0.0f);
            }
        }
    }
    return heights;
}
