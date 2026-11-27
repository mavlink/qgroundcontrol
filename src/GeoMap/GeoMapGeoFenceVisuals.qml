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

/// GeoFence visuals for the GeoMap engine: inclusion/exclusion polygons and
/// circles, the parameter-driven circular fence around home, and the breach
/// return point. While interactive, the selected polygon or circle fence can be
/// edited on the map and the breach return point dragged. Same colors and
/// conventions as the QtLocation GeoFenceMapVisuals.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var geoFenceController
    property var homePosition           ///< QGeoCoordinate; param circular fence centers here
    property bool interactive: false    ///< The fence the controller marks interactive can be edited
    property var mapView                ///< GeoMapView for the polygon tools toolbar, see GeoMapPolygonVisuals

    readonly property color _borderColor: "orange"
    readonly property int _borderWidthInclusion: 2
    readonly property color _interiorColorExclusion: Qt.alpha("orange", 0.2)
    readonly property real _fenceWallHeightMeters: 121.92   // 400 ft AGL
    readonly property color _fenceWallColor: Qt.alpha("orange", 0.25)

    /// The breach return point marker was clicked while interactive
    signal breachReturnPointClicked()

    Repeater {
        model: root.geoFenceController ? root.geoFenceController.polygons : undefined

        GeoMapPolygonVisuals {
            scene: root.scene
            surfaceModel: root.surfaceModel
            mapPolygon: object
            mapView: root.mapView
            interactive: root.interactive && object.interactive
            borderColor: object.inclusion ? root._borderColor : "transparent"
            borderWidth: root._borderWidthInclusion
            fillColor: object.inclusion ? "transparent" : root._interiorColorExclusion
            extrudeHeightMeters: root._fenceWallHeightMeters
            extrudeColor: root._fenceWallColor
            // The 3D wall does not follow this item's opacity
            node3DOpacity: root.opacity
        }
    }

    Repeater {
        model: root.geoFenceController ? root.geoFenceController.circles : undefined

        GeoMapCircleVisuals {
            scene: root.scene
            surfaceModel: root.surfaceModel
            mapCircle: object
            interactive: root.interactive && object.interactive
            borderColor: object.inclusion ? root._borderColor : "transparent"
            borderWidth: root._borderWidthInclusion
            fillColor: object.inclusion ? "transparent" : root._interiorColorExclusion
            extrudeHeightMeters: root._fenceWallHeightMeters
            extrudeColor: root._fenceWallColor
            node3DOpacity: root.opacity
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
        node3DOpacity: root.opacity
    }

    // Breach return point
    GeoMapMissionLabel {
        objectName: "geoMapBreachReturnPoint"
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root.geoFenceController ? root.geoFenceController.breachReturnPoint : QtPositioning.coordinate()
        visible: root.geoFenceController && root.geoFenceController.breachReturnSupported
                 && root.geoFenceController.breachReturnPoint.isValid
        checked: true
        label: qsTr("B", "Breach Return Point item indicator")
        node3DOpacity: root.opacity
        pickable: root.interactive
        draggable: root.interactive
        onClicked: root.breachReturnPointClicked()
        // The drag only moves the point horizontally, keeping its altitude
        onDragged: (coordinate) => root.geoFenceController.breachReturnPoint = QtPositioning.coordinate(
                       coordinate.latitude, coordinate.longitude, root.geoFenceController.breachReturnPoint.altitude)
    }
}
