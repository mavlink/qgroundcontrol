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

    property var parentToolInsets
    property int _widgetMargin: 0
    property bool cameraActive: QGroundControl.videoManager.decoding || QGroundControl.videoManager.isUvc

    Item {
        id: noVideo
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: "black"
        }

        Rectangle {
            id:                 noVideoLabelBackground
            anchors.centerIn:   parent
            width:              noVideoLabel.contentWidth + ScreenTools.defaultFontPixelHeight
            height:             noVideoLabel.contentHeight + ScreenTools.defaultFontPixelHeight
            radius:             ScreenTools.defaultFontPixelWidth / 2
            color:              "white"
            opacity:            0.3
            visible:            !cameraActive
        }

        QGCLabel {
            id:                 noVideoLabel
            text:               qsTr("NO VIDEO AVAILABLE")
            font.bold:          true
            color:              "white"
            font.pointSize:     ScreenTools.smallFontPointSize //ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
    }
    

    SVCameraWidgetLayer {
        id: widgetLayer
        anchors.fill: parent
        anchors.margins: _widgetMargin
        parentToolInsets: root.parentToolInsets
        visible: SVState.svHUD
    }


    
}

