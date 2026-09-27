/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PatchSampler.h"

#include <algorithm>
#include <cmath>

#include "BilinearUV.h"

double PatchSampler::heightAtUV(const ElevationTilePyramid::Grid& grid, double u, double v)
{
    const auto at = [&grid](int x, int y) { return double(grid.heights[(qsizetype(y) * grid.width) + x]); };
    return bilinearAtUV(grid.width, grid.height, u, v, at);
}

PatchSampler::PatchSampler(const ElevationTilePyramid& pyramid, const TileMath::TileKey& key, int gridSize,
                           EdgeStep* edgeStep)
    : _pyramid(pyramid)
    , _key(key)
    , _gridSize(gridSize)
    , _shiftToMax(TileMath::kMaxZoom - key.zoom)
    , _patchView(pyramid.bestTileFor(key))
    , _edgeStep(edgeStep)
{}

QList<float> PatchSampler::sample()
{
    const int verticesPerEdge = _gridSize + 1;
    _north.resize(verticesPerEdge);
    _south.resize(verticesPerEdge);
    _west.resize(verticesPerEdge);
    _east.resize(verticesPerEdge);

    QList<float> heights;
    heights.reserve(qsizetype(verticesPerEdge) * verticesPerEdge);
    bool mismatched = false;
    for (int row = 0; row <= _gridSize; row++) {
        const qint64 m = (qint64(_key.y) * _gridSize) + row;
        for (int col = 0; col <= _gridSize; col++) {
            const qint64 n = (qint64(_key.x) * _gridSize) + col;
            const float own = _patchView.isValid() ? _viewHeight(_patchView, n, m) : 0.0f;
            if ((row > 0) && (row < _gridSize) && (col > 0) && (col < _gridSize)) {
                heights.append(own);
                continue;
            }
            const BoundaryVertex vertex = _boundaryVertex(n, m, own);
            heights.append(vertex.height);
            mismatched = mismatched || (vertex.mismatch != 0.0f);
            if (row == 0) {
                _north[col] = vertex;
            }
            if (row == _gridSize) {
                _south[col] = vertex;
            }
            if (col == 0) {
                _west[row] = vertex;
            }
            if (col == _gridSize) {
                _east[row] = vertex;
            }
        }
    }
    if (!mismatched) {
        return heights;
    }

    QList<float> corrections(heights.size(), 0.0f);
    for (int row = 1; row < _gridSize; row++) {
        for (int col = 1; col < _gridSize; col++) {
            const qsizetype index = (qsizetype(row) * verticesPerEdge) + col;
            corrections[index] = _interiorCorrection(row, col);
            heights[index] += corrections[index];
        }
    }
    if (_edgeStep) {
        _trackEdgeStep(corrections);
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

PatchSampler::BoundaryVertex PatchSampler::_boundaryVertex(qint64 n, qint64 m, float own)
{
    qint64 xCells[2];
    qint64 yCells[2];
    const int xCount = _touchingCells(n << _shiftToMax, xCells);
    const int yCount = _touchingCells(m << _shiftToMax, yCells);

    // Fixed candidate order derived purely from the position: every patch
    // sharing this vertex walks the same cells and returns the first
    // resolvable view, so the height is canonical. No stored data touching
    // the vertex leaves it at zero, which every sharer agrees on.
    BoundaryVertex vertex;
    for (int yi = 0; (yi < yCount) && (vertex.zoom < 0); yi++) {
        for (int xi = 0; xi < xCount; xi++) {
            const ElevationTilePyramid::View view = _resolveCell(xCells[xi], yCells[yi]);
            if (view.isValid()) {
                vertex.height = _viewHeight(view, n, m);
                vertex.zoom = view.key.zoom;
                break;
            }
        }
    }
    const float mismatch = vertex.height - own;
    vertex.mismatch = std::isfinite(mismatch) ? mismatch : 0.0f;  // one bad sample must not smear the interior
    return vertex;
}

/// Convex blend of the four edges' mismatches in this vertex's row and
/// column, weighted by inverse-square distance to each edge: tends to each
/// edge's value at that edge, never overshoots, and ramps a neighbor's
/// differing data in across the patch
float PatchSampler::_interiorCorrection(int row, int col) const
{
    const auto weight = [](int distance) { return 1.0 / (double(distance) * distance); };
    const double north = weight(row);
    const double south = weight(_gridSize - row);
    const double west = weight(col);
    const double east = weight(_gridSize - col);
    const double sum = (north * _north[col].mismatch) + (south * _south[col].mismatch) + (west * _west[row].mismatch) +
                       (east * _east[row].mismatch);
    return static_cast<float>(sum / (north + south + west + east));
}

void PatchSampler::_trackEdgeStep(const QList<float>& corrections)
{
    const int ownZoom = _patchView.isValid() ? _patchView.key.zoom : -1;
    const int cornerSpan = _gridSize / 4;
    for (int row = 0; row <= _gridSize; row++) {
        const bool fullRow = (row == 0) || (row == _gridSize);
        for (int col = 0; col <= _gridSize; col += (fullRow ? 1 : _gridSize)) {
            const int along = fullRow ? col : row;
            if (std::min(along, _gridSize - along) < cornerSpan) {
                continue;
            }
            const BoundaryVertex& vertex = (row == 0)           ? _north[col]
                                           : (row == _gridSize) ? _south[col]
                                           : (col == 0)         ? _west[row]
                                                                : _east[row];
            // Same-zoom neighbors differ only by edge clamping of real data: not a cliff
            if (vertex.zoom == ownZoom) {
                continue;
            }
            float inward = 0.0f;
            if (_gridSize > 1) {
                const int inwardRow = std::clamp(row, 1, _gridSize - 1);
                const int inwardCol = std::clamp(col, 1, _gridSize - 1);
                inward = corrections[(qsizetype(inwardRow) * (_gridSize + 1)) + inwardCol];
            }
            const float step = std::abs(vertex.mismatch - inward);
            if (!(step > _edgeStep->step)) {
                continue;
            }
            _edgeStep->step = step;
            _edgeStep->row = row;
            _edgeStep->col = col;
            _edgeStep->ownZoom = ownZoom;
            _edgeStep->boundaryZoom = vertex.zoom;
        }
    }
}
