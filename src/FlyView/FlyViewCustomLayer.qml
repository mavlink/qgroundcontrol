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
    id: _root

    property var parentToolInsets
    property var totalToolInsets: _toolInsets
    property var mapControl

    QGCToolInsets {
        id: _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    Text {
        text: QGroundControl.multiVehicleManager.activeVehicle
              ? "Vehicle connected"
              : "No vehicle"
        color: "white"
        anchors {
            right: parent.right
            top: parent.top
        }
    }

    Item {
        width: 400
        height: 250

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }

        Rectangle {
            color: "gray"
            opacity: 0.5
            anchors.fill: parent
        }        

        DigitalJoystick {
            anchors {
                left: parent.left
                margins: 5
            }
            height: parent.height
            width: height
            _t: 0.5
            _joystickColor: "black"
            _strokeWidth: 3
            _strokeColor: "white"
                
        }
    } 

    
}