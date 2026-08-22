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
import QtPositioning

import QGroundControl.GeoMap

/// Vertical fence curtain rendered from the terrain up to heightAglMeters
/// along a closed coordinate ring (visible in 3D mode only; flattens away in
/// 2D). Backs the extrudeHeightMeters mode of GeoMapPolygon/GeoMapCircle.
GeoMapItem {
    id: root

    property var coordinates: []        ///< QGeoCoordinate ring vertices
    property real heightAglMeters: 0
    property color color: "orange"

    // Bridges the wall geometry's anchor out of the 3D delegate (see GeoMapFlightPath)
    property var _wallGeometry: null

    altitudeMode: GeoMapItem.Absolute
    crossfade3D: false      // wall flattens to zero height with terrainScale instead
    coordinate: (_wallGeometry && _wallGeometry.anchorCoordinate.isValid)
                ? _wallGeometry.anchorCoordinate
                : QtPositioning.coordinate()

    delegate3D: Component {
        // z offsets are relative altitudes; flatten them with the terrain
        // during the 2D<->3D transition (anchor z already tracks
        // terrainScale through GeoMapItem)
        Node {
            scale: Qt.vector3d(1, 1, root.scene ? root.scene.terrainScale : 1)

            Model {
                geometry: FenceWallGeometry {
                    id: wallGeometry
                    scene: root.scene
                    surfaceModel: root.surfaceModel
                    heightAglMeters: root.heightAglMeters
                    // Skip the geometry work entirely while hidden
                    coordinates: (root.visible && root.coordinates) ? root.coordinates : []
                }

                materials: PrincipledMaterial {
                    lighting: PrincipledMaterial.NoLighting
                    baseColor: root.color
                    alphaMode: PrincipledMaterial.Blend
                    cullMode: Material.NoCulling
                }

                Component.onCompleted: root._wallGeometry = wallGeometry
                Component.onDestruction: root._wallGeometry = null
            }
        }
    }
}
