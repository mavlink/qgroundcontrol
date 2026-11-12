/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

import "GeoMapAltitude.js" as GeoMapAltitude

/// Point marker for complex mission item visuals: same colored dot that
/// crossfades into a 3D sphere as GeoMapWaypointItem, plus an always-visible
/// side text label (e.g. "Loiter", "Land").
GeoMapItem {
    id: root

    property int index: -1     ///< Sequence number shown in the dot; < 0 shows none
    property string label: ""
    property bool checked: false
    property real size: ScreenTools.defaultFontPixelHeight * 1.5
    property bool pickable: false   ///< The marker takes clicks and drags; false lets them through to the map
    property bool draggable: false  ///< The marker can be dragged; the map is not panned meanwhile
    property bool groundMarker: false ///< A position on the ground: lies flat on the terrain in 3D instead of a sphere
    property var dragHeightAt: null   ///< function(coordinate): height in meters at a position, for markers whose height follows it

    // Same conventions as GeoMapWaypointItem / MissionItemIndexLabel:
    // checked is "green" and grown, everything else the plain indicator color
    readonly property color _markerColor: root.checked ? "green" : qgcPal.mapIndicator
    readonly property real _indicatorSize: root.checked ? root.size : root.size * 0.75

    readonly property var _camera: scene ? scene.camera : null

    // Constant apparent size, same technique as GeoMapWaypointItem: scene
    // units per screen pixel at the camera's look-at distance. Built-in
    // QtQuick3D meshes are 100 units across.
    readonly property real _modelScale: _camera
        ? root._indicatorSize * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 1

    readonly property real _groundOpacity: root.groundMarker && root.scene ? root.scene.terrainScale : 0

    // Ground meters for the dot's radius at the marker's own camera distance; the scene is in mercator units,
    // which are 1/cos(latitude) ground meters
    readonly property real _groundRadiusMeters: {
        if (!root.groundMarker || !root._camera || !root.coordinate.isValid) {
            return 0
        }
        const markerDistance = root._camera.scenePosition.minus(root.scenePosition).length()
        return (root._indicatorSize / 2) * markerDistance * root._camera.unitsPerPixelAtUnitDistance
               * Math.cos(root.coordinate.latitude * Math.PI / 180)
    }

    signal clicked()
    signal dragged(var coordinate)  ///< Position the marker was dragged to
    signal dragFinished()

    objectName: "geoMapMissionLabel"
    width: size
    height: size
    anchorPoint: Qt.point(width / 2, height / 2)
    crossfade3D: !groundMarker
    delegate3D: groundMarker ? null : sphereComponent
    // altitudeMode defaults to ClampToGround (GeoMapItem); callers may override

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    // A ground marker lies flat on the terrain in 3D, the same size as the dot it replaces
    GeoMapCircle {
        objectName: "geoMapGroundMarkerCircle"
        x: -root.x
        y: -root.y
        visible: root.groundMarker && opacity > 0
        opacity: root._groundOpacity
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root.coordinate
        radiusMeters: root._groundRadiusMeters
        strokeColor: "transparent"
        strokeWidth: 0
        fillColor: root._markerColor
    }

    Rectangle {
        anchors.centerIn: parent
        width: root._indicatorSize
        height: root._indicatorSize
        radius: width / 2
        color: root._markerColor
        opacity: root.groundMarker ? 1 - root._groundOpacity : root.contentOpacity2D
    }

    QGCLabel {
        anchors.centerIn: parent
        // MissionItemIndexLabel convention: single-char label lives inside the dot
        text: root.index >= 0 ? root.index : (root.label.length === 1 ? root.label : "")
        color: "white"
        font.bold: true
    }

    GeoMapMarkerMouseArea {
        anchors.fill: parent
        marker: root
        enabled: root.pickable
        draggable: root.draggable

        onMarkerClicked: root.clicked()
        onDragFinished: root.dragFinished()
        onDragged: (target) => {
            // Absolute markers sit on the horizontal plane at their altitude, ground-clamped ones on the rendered terrain
            const camera = root.scene.camera
            const coordinate = root.altitudeMode === GeoMapItem.Absolute
                ? GeoMapAltitude.coordinateUnderCursor(camera, target, root.scenePosition.z,
                                                       root.scene.verticalScale * root.scene.terrainScale, root.dragHeightAt)
                : root.surfaceModel.surfaceCoordinateAtScreenPoint(camera, target, root.scene.verticalScale * root.scene.terrainScale)
            if (coordinate.isValid) {
                root.dragged(coordinate)
            }
        }
    }

    GeoMapDropShadow {
        id: dropShadow
        objectName: "geoMapDropShadow"
        geoItem: root
        indicatorSize: root._indicatorSize
    }

    Rectangle {
        anchors.left: parent.right
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.verticalCenter: parent.verticalCenter
        width: sideLabel.width + ScreenTools.defaultFontPixelWidth
        height: sideLabel.height
        radius: height / 2
        color: root._markerColor
        visible: root.label.length > 1

        QGCLabel {
            id: sideLabel
            anchors.centerIn: parent
            text: root.label
            color: "white"
        }
    }

    Component {
        id: sphereComponent

        Node {
            Node {
                scale: Qt.vector3d(root._modelScale, root._modelScale, root._modelScale)

                Model {
                    source: "#Sphere"

                    materials: PrincipledMaterial {
                        baseColor: root._markerColor
                    }
                }
            }

            GeoMapDropLine {
                dropLength: dropShadow.dropLength
                lineScale: dropShadow.dropLineScale
                lineColor: root._markerColor
            }
        }
    }
}
