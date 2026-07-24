import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15
import QtQuick.Effects  // <--- ADD THIS LINE

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property int _widgetMargin: 0
    property bool cameraActive: SVState.synclairOverlay
    && !(QGroundControl.digiviewManager.connected && QGroundControl.videoManager.decoding)

    property int cameraSlot
    property bool previewMode: false
    property real _noVideoLabelPadding: previewMode ? SVUnits.margin : SVUnits.bigMargin
    property real _noVideoLabelPointSize: previewMode ? SVUnits.smallText : SVUnits.mediumText
    readonly property bool cursorTrackingSessionCamera: SVState.cursorTrackingSessionActive
        && SVState.cursorTrackingSessionSlot === cameraSlot
    readonly property bool cursorTrackingOtherCamera: SVState.cursorTrackingSessionActive
        && !cursorTrackingSessionCamera

    signal cursorTargetSelected(int cameraSlot, real normalizedX, real normalizedY)

    readonly property bool crosshair: cameraSlot >= 0
        && cameraSlot < SVState.cameraOverlays.length
        && SVState.cameraOverlays[cameraSlot].crosshair
    readonly property bool grid: cameraSlot >= 0
        && cameraSlot < SVState.cameraOverlays.length
        && SVState.cameraOverlays[cameraSlot].grid

    readonly property url lockedTextureSource: Qt.resolvedUrl("../Resources/Images/halftone_texture_2.png")
    readonly property int lockedTextureTileSize: 250

    QGCPalette { id: qgcPalette}

    Rectangle {
        id: noVideo
        anchors.fill: parent
        color: "black"
        visible: cameraActive

        Rectangle {
            id:                 noVideoLabelBackground
            anchors.centerIn:   parent
            width:              noVideoLabel.contentWidth + root._noVideoLabelPadding * 2
            height:             noVideoLabel.contentHeight + root._noVideoLabelPadding * 2
            radius:             SVUnits.radius
            color:              qgcPalette.windowTransparent

            QGCLabel {
                id:                 noVideoLabel
                text:               qsTr("NO VIDEO AVAILABLE")
                font.bold:          true
                color:              qgcPalette.text
                font.pointSize:     root._noVideoLabelPointSize
                anchors.centerIn:   parent
            }
        }
    }

    SVCameraLayerOverlays {
        id: overlays
        anchors.fill: parent
        grid: parent.grid
        crosshair: parent.crosshair
        //visible: !SVState.cursorTrackingSessionActive
        borderWidth: 3
        borderColor: "black"
        visible: false
    }

    SVCameraLayerOverlays {
        id: overlays2
        anchors.fill: parent
        grid: parent.grid
        crosshair: parent.crosshair
        visible: !SVState.cursorTrackingSessionActive
        borderWidth: 1
        borderColor: "white"
    }

    SVCameraWidgetLayer {
        id: widgetLayer
        anchors.fill: parent
        anchors.margins: SVUnits.bigMargin
        visible: !root.previewMode && SVState.hud && !SVState.cursorTrackingSessionActive
    }

    SVBackground {
        anchors.fill: parent
        visible: !root.previewMode && root.cursorTrackingOtherCamera
        effect: false
    }

    Item {
            id: lockOverlay
            anchors.fill: parent
            anchors.margins: SVUnits.margin / 2
            visible: !root.previewMode && root.cursorTrackingOtherCamera

            Image {
                id: lockOverlayTexture
                anchors.fill: parent
                fillMode: Image.Tile
                sourceSize.width: root.lockedTextureTileSize
                sourceSize.height: root.lockedTextureTileSize
                visible: false
                source: root.lockedTextureSource
            }

            MultiEffect {
                anchors.fill: parent
                source: lockOverlayTexture
                maskEnabled: true
                maskSource: lockOverlayMask
                opacity: 0.05
            }

            Item {
                id: lockOverlayMask
                anchors.fill: parent
                layer.enabled: true
                visible: false

                Rectangle {
                    anchors.fill: parent
                    color: "black"
                }
            }
        }

    SVBorder {
        id: trackingSelected
        anchors.fill: parent
        borderWidth: 3
        borderColor: "white"
        borderVisible: !root.previewMode && cursorTrackingSessionCamera
    }

    

    SVBorder {
        id: selected
        anchors.fill: parent
        borderWidth: SVUnits.thickLineWidth + SVUnits.lineWidth * 2
        borderColor: qgcPalette.colorYellowGreen
        borderVisible: !root.previewMode
            && SVState.cameraSelectionEnabled
            && SVState.cameraSelected === cameraSlot
            && !SVState.cursorTrackingSessionActive
            && SVState.hud 

    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.previewMode && !SVState.cursorTrackingSessionActive && SVState.cameraSelectionEnabled && SVState.hud
        onClicked: {
            if (SVState.lockControls) {
                SVState.clearCamera()
                return
            }

            SVState.setCamera(root.cameraSlot)
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.previewMode && SVState.cursorTrackingSessionActive

        onClicked: (mouse) => {
            mouse.accepted = true

            if (root.cursorTrackingSessionCamera) {
                if (width > 0 && height > 0) {
                    root.cursorTargetSelected(root.cameraSlot, mouse.x / width, mouse.y / height)
                } else {
                    SVState.cancelCursorTrackingSelection()
                }
                return
            }

            SVState.cancelCursorTrackingSelectionFromBackground()
        }
    }
}
