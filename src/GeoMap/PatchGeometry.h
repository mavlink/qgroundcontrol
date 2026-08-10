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
#include <QtQuick3D/QQuick3DGeometry>

/// Grid mesh for one surface patch of the GeoMap engine.
///
/// Local space: origin at the patch center, x east, y north, z up (meters).
/// The grid has (gridSize+1)^2 vertices spanning [-span/2, +span/2] in x/y,
/// displaced in z by the height grid (row-major from the north-west corner,
/// matching HeightSource). Empty heights produce a flat patch at z=0.
///
/// Skirt geometry hangs from all four edges to hide cracks between adjacent
/// patches of different LOD levels. UVs map (0,0) at the north-west corner to
/// (1,1) at the south-east corner, matching raster tile orientation.
///
/// Known limitation: normals are central differences over this patch's own
/// height grid (one-sided at edges), so lit materials show shading seams at
/// patch borders. Fix when lit terrain matters (M2): deliver a one-sample
/// apron from the HeightSource so edge normals use neighboring data.
class PatchGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(qreal span READ span WRITE setSpan NOTIFY spanChanged)
    Q_PROPERTY(QList<float> heights READ heights WRITE setHeights NOTIFY heightsChanged)

public:
    explicit PatchGeometry(QQuick3DObject* parent = nullptr);

    static constexpr int kMinGridSize = 1;
    static constexpr int kMaxGridSize = 256;
    static constexpr double kSkirtDepthFraction = 0.05;  ///< skirt depth as fraction of span

    int gridSize() const { return _gridSize; }

    void setGridSize(int gridSize);

    qreal span() const { return _span; }

    void setSpan(qreal span);

    /// (gridSize+1)^2 heights row-major from the north-west corner; empty = flat
    QList<float> heights() const { return _heights; }

    void setHeights(const QList<float>& heights);

signals:
    void gridSizeChanged();
    void spanChanged();
    void heightsChanged();

private:
    void _rebuild();
    float _heightAt(int row, int col) const;

    int _gridSize = 16;
    qreal _span = 1000.0;
    QList<float> _heights;
};
