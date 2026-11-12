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
    property color fillColor: "transparent"
    property real extrudeHeightMeters: 0    ///< 3D fence wall height, see GeoMapCircle
    property color extrudeColor: Qt.alpha(borderColor, 0.25)
    property real node3DOpacity: 1          ///< Dimming of the wall, see GeoMapItem.node3DOpacity
    property bool centerDragHandleVisible: true
    property bool radiusLabelVisible: false
    property int altitudeMode: GeoMapItem.ClampToGround

    readonly property var _center: mapCircle ? mapCircle.center : QtPositioning.coordinate()
    readonly property real _radius: mapCircle ? mapCircle.radius.rawValue : 0
    readonly property var _camera: scene ? scene.camera : null

    GeoMapCircle {
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root._center
        radiusMeters: root._radius
        strokeColor: root.borderColor
        strokeWidth: root.borderWidth
        fillColor: root.fillColor
        extrudeHeightMeters: root.extrudeHeightMeters
        extrudeColor: root.extrudeColor
        node3DOpacity: root.node3DOpacity
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

    GeoMapDragHandle {
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

    GeoMapDragHandle {
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
