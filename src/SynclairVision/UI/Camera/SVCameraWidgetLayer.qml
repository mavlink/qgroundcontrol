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

    SVBackground {
        width: compass.width + 6
        height: compass.height + 6
        
        enabled: true
        borderColor: qgcPalette.windowShade
        hoverEnabled: false
        checkable: false
        checked: false
        hovered: false
        pressed: false
        borderWidth: 1
        radius: height / 2

        anchors.right: parent.right
        anchors.bottom: parent.bottom

        HorizontalCompassAttitude {
            id: compass
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 200
            color: "transparent"
        }
    }
}

    

