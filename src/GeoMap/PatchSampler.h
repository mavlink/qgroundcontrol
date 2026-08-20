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
#include <QtCore/QVarLengthArray>

#include "ElevationTilePyramid.h"
#include "TileMath.h"

/// Canonical vertex-height resolution for one patch: interior vertices
/// sample the patch's own backing view; boundary vertices resolve by
/// position so every patch sharing the vertex samples bit-identical heights
/// (see HeightField::samplePatch). gridSize must be a power of two so
/// vertex UVs are exact dyadic doubles. Single-use: construct, sample() once.
class PatchSampler
{
public:
    PatchSampler(const ElevationTilePyramid& pyramid, const TileMath::TileKey& key, int gridSize);

    /// The (gridSize+1)^2 vertex heights, row-major from the NW corner
    QList<float> sample();

    /// Bilinear height at a unit-UV position within a grid (origin NW corner),
    /// sample-center convention, clamped at grid edges (see BilinearUV.h)
    static double heightAtUV(const ElevationTilePyramid::Grid& grid, double u, double v);

private:
    float _viewHeight(const ElevationTilePyramid::View& view, qint64 n, qint64 m) const;
    float _boundaryHeight(qint64 n, qint64 m);
    ElevationTilePyramid::View _resolveCell(qint64 cx, qint64 cy);
    int _touchingCells(qint64 s, qint64 (&cells)[2]) const;

    /// Memo of resolved views per key.zoom tile: when a tile has no stored
    /// descendant, every cell inside it resolves identically (chain below the
    /// tile is empty, ancestors are shared), so one lookup answers the whole
    /// edge run along it. An invalid view memoizes the same way — repeated
    /// misses over uncovered neighbors stay O(1). A patch's boundary touches
    /// at most 9 such tiles (own + 8 neighbors).
    struct TileMemo
    {
        TileMath::TileKey tile;
        ElevationTilePyramid::View view;
    };

    const ElevationTilePyramid& _pyramid;
    const TileMath::TileKey _key;
    const int _gridSize;
    const int _shiftToMax;
    const ElevationTilePyramid::View _patchView;
    QVarLengthArray<TileMemo, 9> _memos;
};
