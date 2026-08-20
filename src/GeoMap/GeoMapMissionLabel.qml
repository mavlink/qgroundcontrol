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

/// Point marker for complex mission item visuals: same colored dot that
/// crossfades into a 3D sphere as GeoMapWaypointItem, plus an always-visible
/// side text label (e.g. "Loiter", "Land").
GeoMapItem {
    id: root

    property int index: -1     ///< Sequence number shown in the dot; < 0 shows none
    property string label: ""
    property bool checked: false
    property real size: ScreenTools.defaultFontPixelHeight * 1.5

    width: size
    height: size
    anchorPoint: Qt.point(width / 2, height / 2)
    // altitudeMode defaults to ClampToGround (GeoMapItem); callers may override

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

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

    Rectangle {
        anchors.centerIn: parent
        width: root._indicatorSize
        height: root._indicatorSize
        radius: width / 2
        color: root._markerColor
        opacity: root.contentOpacity2D
    }

    QGCLabel {
        anchors.centerIn: parent
        text: root.index >= 0 ? root.index : ""
        color: "white"
        font.bold: true
    }

    Rectangle {
        anchors.left: parent.right
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.verticalCenter: parent.verticalCenter
        width: sideLabel.width + ScreenTools.defaultFontPixelWidth
        height: sideLabel.height
        radius: height / 2
        color: root._markerColor
        visible: root.label.length > 0

        QGCLabel {
            id: sideLabel
            anchors.centerIn: parent
            text: root.label
            color: "white"
        }
    }

    delegate3D: Component {
        Node {
            scale: Qt.vector3d(root._modelScale, root._modelScale, root._modelScale)

            Model {
                source: "#Sphere"

                materials: PrincipledMaterial {
                    baseColor: root._markerColor
                }
            }
        }
    }
}
