/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Shapes

import QGroundControl
import QGroundControl.GeoMap

/// Polygon outline/fill from a coordinate list: the GeoMap counterpart of
/// QGCMapPolygonVisuals (display only — no drag-to-edit vertices).
/// Ground-projected by default; set altitudeMode to GeoMapItem.Absolute to
/// render at the vertex coordinates' altitude. Set extrudeHeightMeters to
/// also render a 3D fence wall from the terrain up to that AGL height
/// (visible in 3D mode only; flattens away in 2D).
Item {
    id: root

    property var scene
    property var surfaceModel
    property var coordinates: []        ///< QGeoCoordinate vertices, unclosed ring
    property color strokeColor: QGroundControl.globalPalette.mapMissionTrajectory
    property real strokeWidth: 3
    property color fillColor: "transparent"
    property real extrudeHeightMeters: 0    ///< 0 = flat outline only
    property color extrudeColor: Qt.alpha(strokeColor, 0.25)
    property alias altitudeMode: pathProjector.altitudeMode

    GeoMapProjectedPath {
        id: pathProjector
        scene: root.scene
        surfaceModel: root.surfaceModel
        // Skip the projection work entirely while hidden; close the ring
        coordinates: (root.visible && root.coordinates && root.coordinates.length > 2)
                     ? root.coordinates.concat([root.coordinates[0]])
                     : []
    }

    Shape {
        visible: pathProjector.projected

        ShapePath {
            strokeColor: root.strokeColor
            strokeWidth: root.strokeWidth
            fillColor: root.fillColor
            PathPolyline { path: pathProjector.screenPoints }
        }
    }

    GeoMapFenceWall {
        visible: root.visible && root.extrudeHeightMeters > 0
        scene: root.scene
        surfaceModel: root.surfaceModel
        heightAglMeters: root.extrudeHeightMeters
        color: root.extrudeColor
        coordinates: root.coordinates
    }
}
