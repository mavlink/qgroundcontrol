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
import QtQuick.Shapes
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// GeoMap counterpart of QGCMapCircleVisuals: a QGCMapCircle outline with
/// rotation arrows and, while interactive, center/radius drag handles.
/// No isValid gating needed: GeoMapItem parks invalid coordinates off-screen.
Item {
    id: root

    property var scene
    property var surfaceModel
    property var mapCircle                                              ///< QGCMapCircle object
    property bool interactive: mapCircle ? mapCircle.interactive : false
    property color borderColor: QGroundControl.globalPalette.mapMissionTrajectory
    property real borderWidth: 3
    property bool centerDragHandleVisible: true
    property bool radiusLabelVisible: false
    property int altitudeMode: GeoMapItem.ClampToGround

    readonly property var _center: mapCircle ? mapCircle.center : QtPositioning.coordinate()
    readonly property real _radius: mapCircle ? mapCircle.radius.rawValue : 0
    readonly property var _camera: scene ? scene.camera : null

    /// Screen-space handle that reports the coordinate it is dragged to, on the
    /// same surface the handle is drawn on so it stays under the cursor at any tilt
    component DragHandle: GeoMapItem {
        id: handle

        property bool labelVisible: false
        property string labelText
        property bool showGroundDrop: false     ///< Drop line and ground shadow under a floating handle

        signal dragged(var coordinate)

        width: ScreenTools.defaultFontPixelHeight * 1.5
        height: width
        anchorPoint: Qt.point(width / 2, height / 2)
        // Handle stays opaque in 3D; its 3D node is only the drop line
        crossfade3D: false
        delegate3D: showGroundDrop ? dropLineComponent : null

        Component {
            id: dropLineComponent

            Node {
                GeoMapDropLine {
                    dropLength: dropShadow.dropLength
                    lineScale: dropShadow.dropLineScale
                }
            }
        }

        GeoMapDropShadow {
            id: dropShadow
            objectName: "geoMapDragHandleDropShadow"
            geoItem: handle
            indicatorSize: handle.width
            visible: handle.showGroundDrop && dropLength > 0
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "white"
            opacity: 0.9
        }

        Rectangle {
            anchors.bottom: parent.top
            anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.25
            anchors.horizontalCenter: parent.horizontalCenter
            width: handleLabel.width + (ScreenTools.defaultFontPixelHeight * 0.5)
            height: parent.height
            radius: height / 2
            color: "white"
            opacity: 0.5
            visible: handle.labelVisible
        }

        QGCLabel {
            id: handleLabel
            anchors.bottom: parent.top
            anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.25
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            color: "black"
            text: handle.labelText
            visible: handle.labelVisible
        }

        MouseArea {
            anchors.fill: parent
            // The map pan DragHandler would otherwise take over past the drag threshold
            preventStealing: true

            // Cursor-to-anchor offset at press, so the handle doesn't jump to the cursor
            property point _grabOffset

            function _parentPosition(mouse) {
                return handle.mapToItem(handle.parent, mouse.x, mouse.y)
            }

            onPressed: (mouse) => {
                const pos = _parentPosition(mouse)
                _grabOffset = Qt.point(handle.x + handle.anchorPoint.x - pos.x, handle.y + handle.anchorPoint.y - pos.y)
            }

            onPositionChanged: (mouse) => {
                const pos = _parentPosition(mouse)
                const target = Qt.point(pos.x + _grabOffset.x, pos.y + _grabOffset.y)
                const scene = handle.scene
                // Absolute handles sit on the horizontal plane at their altitude,
                // ground-clamped ones on the rendered terrain
                const coordinate = (handle.altitudeMode === GeoMapItem.Absolute)
                    ? scene.camera.coordinateAtScreenPoint(target, handle.scenePosition.z)
                    : handle.surfaceModel.surfaceCoordinateAtScreenPoint(scene.camera, target, scene.verticalScale * scene.terrainScale)
                if (coordinate.isValid) {
                    handle.dragged(coordinate)
                }
            }
        }
    }

    GeoMapCircle {
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root._center
        radiusMeters: root._radius
        strokeColor: root.borderColor
        strokeWidth: root.borderWidth
        altitudeMode: root.altitudeMode
    }

    // Rotation direction arrows at the north (index 0) and south of the ring
    Repeater {
        model: 2

        GeoMapItem {
            id: rotationIndicator
            objectName: "geoMapCircleRotationIndicator"

            required property int index

            scene: root.scene
            surfaceModel: root.surfaceModel
            altitudeMode: root.altitudeMode
            coordinate: root._center.isValid ? root._center.atDistanceAndAzimuth(root._radius, index === 0 ? 0 : 180) : QtPositioning.coordinate()
            width: ScreenTools.defaultFontPixelHeight
            height: width
            anchorPoint: Qt.point(width / 2, height / 2)
            visible: root.mapCircle ? root.mapCircle.showRotation : false

            Shape {
                anchors.fill: parent
                // Camera heading rotates map north on screen; the arrows follow the ring
                rotation: (root.mapCircle && root.mapCircle.clockwiseRotation ? 0 : 180) + (rotationIndicator.index === 0 ? 180 : 0)
                          + (root._camera ? root._camera.heading : 0)

                ShapePath {
                    strokeWidth: 2
                    strokeColor: root.borderColor
                    fillColor: root.borderColor
                    startX: 0
                    startY: rotationIndicator.width / 2
                    PathLine { x: rotationIndicator.width; y: rotationIndicator.width }
                    PathLine { x: rotationIndicator.width; y: 0 }
                    PathLine { x: 0; y: rotationIndicator.width / 2 }
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.interactive
                onClicked: root.mapCircle.clockwiseRotation = !root.mapCircle.clockwiseRotation
            }
        }
    }

    DragHandle {
        objectName: "geoMapCircleCenterHandle"
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: root.altitudeMode
        coordinate: root._center
        visible: root.interactive && root.centerDragHandleVisible
        showGroundDrop: true
        // Keep the circle's altitude: the drag only moves it horizontally
        onDragged: (coordinate) => root.mapCircle.center = QtPositioning.coordinate(coordinate.latitude, coordinate.longitude, root._center.altitude)
    }

    DragHandle {
        objectName: "geoMapCircleRadiusHandle"
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: root.altitudeMode
        coordinate: root._center.isValid ? root._center.atDistanceAndAzimuth(root._radius, 90) : QtPositioning.coordinate()
        visible: root.interactive
        labelVisible: root.radiusLabelVisible
        labelText: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(root._radius).toFixed(0) + " "
                   + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
        onDragged: (coordinate) => root.mapCircle.radius.rawValue = root._center.distanceTo(coordinate)
    }
}
