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
 
    HorizontalCompassAttitude {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 200
    }
}

