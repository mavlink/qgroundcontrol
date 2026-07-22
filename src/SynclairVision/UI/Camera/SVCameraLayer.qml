import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property int _widgetMargin: 0
    property bool cameraActive: !QGroundControl.videoManager.decoding && !QGroundControl.videoManager.isUvc
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

    QGCPalette { id: qgcPalette}

    Rectangle {
        id: noVideo
        anchors.fill: parent
        color: "black"
        visible: false//cameraActive

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
        visible: !SVState.cursorTrackingSessionActive
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

    

    SVBorder {
        id: selected
        anchors.fill: parent
        borderWidth: SVUnits.thickLineWidth - SVUnits.lineWidth
        borderColor: qgcPalette.colorYellowGreen
        borderVisible: !root.previewMode
            && SVState.cameraSelectionEnabled
            && SVState.cameraSelected === cameraSlot
            && SVState.hud && !SVState.cursorTrackingSessionActive
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.previewMode && !SVState.cursorTrackingSessionActive && SVState.cameraSelectionEnabled
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
