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

 
    SVCameraWidgetLayer {
        id: widgetLayer
        anchors.fill: parent
        anchors.margins: _widgetMargin
        parentToolInsets: root.parentToolInsets
    }

    
}

