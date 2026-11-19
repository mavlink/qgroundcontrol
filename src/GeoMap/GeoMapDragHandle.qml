pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Screen-space handle that reports the coordinate it is dragged to, on the
/// same surface the handle is drawn on so it stays under the cursor at any tilt
GeoMapItem {
    id: handle

    property bool labelVisible: false
    property string labelText
    property bool showGroundDrop: false     ///< Drop line and ground shadow under a floating handle

    property bool _dragging: false

    signal clicked()
    signal dragStarted()
    signal dragged(var coordinate)
    signal dragFinished()

    function _finishDrag() {
        if (_dragging) {
            _dragging = false
            handle.dragFinished()
        }
    }

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

    GeoMapMarkerMouseArea {
        anchors.fill: parent
        marker: handle
        draggable: true

        onMarkerClicked: handle.clicked()
        onDragged: (target) => {
            const scene = handle.scene
            // Absolute handles sit on the horizontal plane at their altitude,
            // ground-clamped ones on the rendered terrain
            const coordinate = (handle.altitudeMode === GeoMapItem.Absolute)
                ? scene.camera.coordinateAtScreenPoint(target, handle.scenePosition.z)
                : handle.surfaceModel.surfaceCoordinateAtScreenPoint(scene.camera, target, scene.verticalScale * scene.terrainScale)
            if (coordinate.isValid) {
                if (!handle._dragging) {
                    handle._dragging = true
                    handle.dragStarted()
                }
                handle.dragged(coordinate)
            }
        }
        onDragFinished: handle._finishDrag()
    }
}
