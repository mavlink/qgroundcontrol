/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.GeoMap

/// GeoFence visuals for the GeoMap engine (display only, no editing):
/// inclusion/exclusion polygons and circles, the parameter-driven circular
/// fence around home, and the breach return point. Same colors and
/// conventions as the QtLocation GeoFenceMapVisuals.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var geoFenceController
    property var homePosition           ///< QGeoCoordinate; param circular fence centers here

    readonly property color _borderColor: "orange"
    readonly property int _borderWidthInclusion: 2
    readonly property color _interiorColorExclusion: Qt.alpha("orange", 0.2)
    readonly property real _fenceWallHeightMeters: 121.92   // 400 ft AGL
    readonly property color _fenceWallColor: Qt.alpha("orange", 0.25)

    Repeater {
        model: root.geoFenceController ? root.geoFenceController.polygons : undefined

        GeoMapPolygon {
            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinates: object.path
            strokeColor: object.inclusion ? root._borderColor : "transparent"
            strokeWidth: root._borderWidthInclusion
            fillColor: object.inclusion ? "transparent" : root._interiorColorExclusion
            extrudeHeightMeters: root._fenceWallHeightMeters
            extrudeColor: root._fenceWallColor
        }
    }

    Repeater {
        model: root.geoFenceController ? root.geoFenceController.circles : undefined

        GeoMapCircle {
            scene: root.scene
            surfaceModel: root.surfaceModel
            center: object.center
            radiusMeters: object.radius.rawValue
            strokeColor: object.inclusion ? root._borderColor : "transparent"
            strokeWidth: root._borderWidthInclusion
            fillColor: object.inclusion ? "transparent" : root._interiorColorExclusion
            extrudeHeightMeters: root._fenceWallHeightMeters
            extrudeColor: root._fenceWallColor
        }
    }

    // Circular geofence specified from vehicle parameter, centered on home
    GeoMapCircle {
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root.homePosition
        radiusMeters: root.geoFenceController ? root.geoFenceController.paramCircularFence : 0
        strokeColor: root._borderColor
        strokeWidth: root._borderWidthInclusion
        extrudeHeightMeters: root._fenceWallHeightMeters
        extrudeColor: root._fenceWallColor
    }

    // Breach return point
    GeoMapMissionLabel {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root.geoFenceController ? root.geoFenceController.breachReturnPoint : QtPositioning.coordinate()
        visible: root.geoFenceController && root.geoFenceController.breachReturnPoint.isValid
        checked: true
        label: qsTr("B", "Breach Return Point item indicator")
    }
}
