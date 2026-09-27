/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <algorithm>
#include <array>

#include <QtCore/QList>

/// The drawn surface of a patch height grid ((gridSize+1)^2 heights, row-major
/// from the NW corner, row 0 north). PatchGeometry draws it and SurfaceModel
/// answers heights on it: sharing these rules keeps items on what is drawn.
namespace PatchMesh {

/// Per-edge LOD deltas {N, S, W, E} (see PatchGeometry::setEdgeLodDeltas);
/// callers pass only deltas whose step divides gridSize
using LodDeltas = std::array<int, 4>;

/// Vertex height as drawn. On an edge with a coarser neighbor, non-coincident
/// vertices collapse onto the segment between the coincident ones (T-junction
/// fix); a corner on two constrained edges takes the first match (N,S,W,E).
/// Out-of-range indices clamp; a grid of the wrong size draws flat.
inline float vertexHeight(const QList<float>& heights, int gridSize, const LodDeltas& lodDeltas, int row, int col)
{
    const int verticesPerEdge = gridSize + 1;
    if (heights.count() != (qsizetype(verticesPerEdge) * verticesPerEdge)) {
        return 0.0f;
    }
    const int r = std::clamp(row, 0, gridSize);
    const int c = std::clamp(col, 0, gridSize);
    const auto raw = [&](int rr, int cc) { return heights.at((qsizetype(rr) * verticesPerEdge) + cc); };

    int edge = -1;
    if ((r == 0) && (lodDeltas[0] > 0)) {
        edge = 0;
    } else if ((r == gridSize) && (lodDeltas[1] > 0)) {
        edge = 1;
    } else if ((c == 0) && (lodDeltas[2] > 0)) {
        edge = 2;
    } else if ((c == gridSize) && (lodDeltas[3] > 0)) {
        edge = 3;
    }
    if (edge < 0) {
        return raw(r, c);
    }
    const bool alongCol = (edge <= 1);  // north/south edges run along columns
    const int step = 1 << lodDeltas[edge];
    const int idx = alongCol ? c : r;
    const int base = (idx / step) * step;
    if (idx == base) {
        return raw(r, c);
    }
    const float t = float(idx - base) / step;
    const float a = alongCol ? raw(r, base) : raw(base, c);
    const float b = alongCol ? raw(r, base + step) : raw(base + step, c);
    return a + ((b - a) * t);
}

/// Drawn height at a fractional grid position (clamped to [0, gridSize]):
/// linear within the triangle PatchGeometry draws there. Each cell's two
/// triangles share the (row, col+1)-(row+1, col) diagonal.
inline double surfaceHeight(const QList<float>& heights, int gridSize, const LodDeltas& lodDeltas, double row,
                            double col)
{
    const double r = std::clamp(row, 0.0, double(gridSize));
    const double c = std::clamp(col, 0.0, double(gridSize));
    const int row0 = std::min(int(r), gridSize - 1);
    const int col0 = std::min(int(c), gridSize - 1);
    const double fr = r - row0;
    const double fc = c - col0;
    const auto h = [&](int dr, int dc) {
        return double(vertexHeight(heights, gridSize, lodDeltas, row0 + dr, col0 + dc));
    };
    if ((fr + fc) <= 1.0) {
        return h(0, 0) + ((h(1, 0) - h(0, 0)) * fr) + ((h(0, 1) - h(0, 0)) * fc);
    }
    return h(1, 1) + ((h(0, 1) - h(1, 1)) * (1.0 - fr)) + ((h(1, 0) - h(1, 1)) * (1.0 - fc));
}

}  // namespace PatchMesh
