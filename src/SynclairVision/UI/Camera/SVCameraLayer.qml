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
    property bool cameraActive: QGroundControl.videoManager.decoding || QGroundControl.videoManager.isUvc
    property int cameraIndex

    QGCPalette { id: qgcPalette}

    Rectangle {
        id: noVideo
        anchors.fill: parent
        color: "black"
        visible:            !cameraActive

        Rectangle {
            id:                 noVideoLabelBackground
            anchors.centerIn:   parent
            width:              noVideoLabel.contentWidth + SVUnits.bigMargin * 2
            height:             noVideoLabel.contentHeight + SVUnits.bigMargin * 2
            radius:             SVUnits.radius
            color:              qgcPalette.windowTransparent

            QGCLabel {
                id:                 noVideoLabel
                text:               qsTr("NO VIDEO AVAILABLE")
                font.bold:          true
                color:              qgcPalette.text
                font.pointSize:     SVUnits.mediumText
                anchors.centerIn:   parent
            }
        }
    }
    
    SVCameraWidgetLayer {
        id: widgetLayer
        anchors.fill: parent
        anchors.margins: SVUnits.bigMargin
        visible: SVState.hud
    }

    Rectangle {
        id: selected
        anchors.fill: parent
        color: "transparent"
        border.width: SVUnits.thickLineWidth - SVUnits.lineWidth
        border.color: qgcPalette.colorYellowGreen
        visible: SVState.cameraSelected === cameraIndex && SVState.hud
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            SVState.cameraSelected = (SVState.cameraSelected === root.cameraIndex || SVState.lockControls)
                ? -1
                : root.cameraIndex
        }
    }
}

