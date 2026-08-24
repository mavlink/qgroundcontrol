/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick3D

/// Drop line from a floating marker to the terrain below (Google Earth-style
/// "extrude"): anchors the marker visually so its altitude is readable.
/// Place inside the marker's delegate3D node; dropLength / lineScale come
/// from the marker's GeoMapDropShadow, which owns the drop computation.
Model {
    id: root

    property real dropLength: 0     ///< Scene-space marker-to-ground distance
    property real lineScale: 0      ///< Diameter scale (GeoMapDropShadow.dropLineScale)
    property color lineColor: "white"

    source: "#Cylinder"   // built-in: 100x100 units, height along +Y
    visible: dropLength > 0
    opacity: 0.75
    eulerRotation.x: 90   // stand the height axis up along scene z
    position: Qt.vector3d(0, 0, -dropLength / 2)
    scale: Qt.vector3d(lineScale, dropLength / 100, lineScale)

    materials: PrincipledMaterial {
        lighting: PrincipledMaterial.NoLighting
        baseColor: root.lineColor
    }
}
