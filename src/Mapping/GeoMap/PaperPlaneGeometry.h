/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQuick3D/QQuick3DGeometry>

/// Paper-airplane dart mesh for the GeoMap vehicle marker: the 3D counterpart
/// of the 2D vehicleArrowOpaque icon (same silhouette). Nose along +y, x
/// east, z up; ~100 units long to match the built-in-mesh scale convention.
/// Two dihedral red wings and a paper-white keel fin (the fuselage) below
/// the spine. Flat-shaded via per-face normals; colors are baked as vertex
/// colors, so render with a material that has vertexColorsEnabled (and
/// NoCulling, since the mesh is single-sided).
class PaperPlaneGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PaperPlaneGeometry(QQuick3DObject* parent = nullptr);

    static constexpr int kTriangleCount = 3;
    static constexpr int kFloatsPerVertex = 10;  ///< position 3, normal 3, color 4
};
