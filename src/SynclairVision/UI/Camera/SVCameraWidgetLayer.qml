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

        QGCPalette { id: qgcPalette }

 
    Rectangle {
        width: compass.width + SVUnits.lineWidth * 2
        height: compass.height + SVUnits.lineWidth * 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        radius: height / 2
        color: qgcPalette.windowShade

        HorizontalCompassAttitude {
            id: compass
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 200
        }
    }
    
}

