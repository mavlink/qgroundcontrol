/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PatchSampler.h"

#include <cmath>

#include "BilinearUV.h"

double PatchSampler::heightAtUV(const ElevationTilePyramid::Grid& grid, double u, double v)
{
    const auto at = [&grid](int x, int y) { return double(grid.heights[(qsizetype(y) * grid.width) + x]); };
    return bilinearAtUV(grid.width, grid.height, u, v, at);
}

PatchSampler::PatchSampler(const ElevationTilePyramid& pyramid, const TileMath::TileKey& key, int gridSize)
    : _pyramid(pyramid),
      _key(key),
      _gridSize(gridSize),
      _shiftToMax(TileMath::kMaxZoom - key.zoom),
      _patchView(pyramid.bestTileFor(key))
{}

QList<float> PatchSampler::sample()
{
    QList<float> heights;
    heights.reserve(qsizetype(_gridSize + 1) * (_gridSize + 1));
    for (int row = 0; row <= _gridSize; row++) {
        const qint64 m = (qint64(_key.y) * _gridSize) + row;
        for (int col = 0; col <= _gridSize; col++) {
            const qint64 n = (qint64(_key.x) * _gridSize) + col;
            if ((row == 0) || (row == _gridSize) || (col == 0) || (col == _gridSize)) {
                heights.append(_boundaryHeight(n, m));
            } else {
                heights.append(_patchView.isValid() ? _viewHeight(_patchView, n, m) : 0.0f);
            }
        }
    }
    return heights;
}

/// Height at the exact vertex position (n/gridSize, m/gridSize in tile units
/// at key.zoom) within a resolved view; ldexp rescales exactly, so equal
/// positions give equal UV bits regardless of the asking patch
float PatchSampler::_viewHeight(const ElevationTilePyramid::View& view, qint64 n, qint64 m) const
{
    const double u = (std::ldexp(double(n), view.key.zoom - _key.zoom) / _gridSize) - view.key.x;
    const double v = (std::ldexp(double(m), view.key.zoom - _key.zoom) / _gridSize) - view.key.y;
    return static_cast<float>(heightAtUV(*view.grid, u, v));
}

/// Cells at kMaxZoom touching a vertex position on one axis, east/south side
/// first; a vertex not exactly on a cell boundary lies in a single cell
int PatchSampler::_touchingCells(qint64 s, qint64 (&cells)[2]) const
{
    const qint64 cellCount = qint64(1) << TileMath::kMaxZoom;
    int count = 0;
    const qint64 cell = s / _gridSize;
    if (cell < cellCount) {
        cells[count++] = cell;
    }
    if (((s % _gridSize) == 0) && (cell > 0)) {
        cells[count++] = cell - 1;
    }
    return count;
}

ElevationTilePyramid::View PatchSampler::_resolveCell(qint64 cx, qint64 cy)
{
    const TileMath::TileKey tile{int(cx >> _shiftToMax), int(cy >> _shiftToMax), _key.zoom};
    for (const TileMemo& memo : _memos) {
        if (memo.tile == tile) {
            return memo.view;
        }
    }
    const ElevationTilePyramid::View view =
        _pyramid.bestTileFor(TileMath::TileKey{int(cx), int(cy), TileMath::kMaxZoom});
    if (!_pyramid.hasDescendant(tile) && (_memos.size() < _memos.capacity())) {
        _memos.append({tile, view});
    }
    return view;
}

float PatchSampler::_boundaryHeight(qint64 n, qint64 m)
{
    qint64 xCells[2];
    qint64 yCells[2];
    const int xCount = _touchingCells(n << _shiftToMax, xCells);
    const int yCount = _touchingCells(m << _shiftToMax, yCells);

    // Fixed candidate order derived purely from the position: every patch
    // sharing this vertex walks the same cells and returns the first
    // resolvable view, so the height is canonical
    for (int yi = 0; yi < yCount; yi++) {
        for (int xi = 0; xi < xCount; xi++) {
            const ElevationTilePyramid::View view = _resolveCell(xCells[xi], yCells[yi]);
            if (view.isValid()) {
                return _viewHeight(view, n, m);
            }
        }
    }
    return 0.0f;  // no stored data touches the vertex: every sharer agrees on zero
}
