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

    QGCButton {
            Layout.fillWidth:   true
            text:               "test"
            
            anchors {
                top: parent.top
                right: parent.right
            }

            onClicked: {
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
            _t: 0.4
            _joystickColor: "black"
            _strokeWidth: 5
            _strokeColor: "white"
                
            onButtonClicked: (id) => {
                var vehicle = QGroundControl.multiVehicleManager.activeVehicle
    console.log("vehicle:", vehicle)
                if (!vehicle || !vehicle.svMavlinkHandler) return
                
                console.log("knapp tryckt")

                var step = 5.0
                var yaw = 0.0
                var pitch = 0.0



                switch (id) {
                    case 0: yaw   = -step; break   // left
                    case 1: yaw   =  step; break   // right
                    case 2: pitch =  step; break   // up
                    case 3: pitch = -step; break   // down
                    case 4: case 5: case 6: case 7:
                        // center quadrants — stop or hover
                        break
                    default: return                 // -1, nothing
                }

                vehicle.svMavlinkHandler.sendCamTargetingParameters(
                    "main",   // stream_name
                    0,        // cam_id
                    1,        // targeting_mode — euler delta mode
                    1,        // euler_delta = true
                    yaw,      // yaw
                    pitch,    // pitch
                    0,        // roll
                    0,        // lock_flags
                    0, 0,     // x_offset, y_offset
                    0, 0, 0,  // target lat/lon/alt (unused in delta mode)
                    -1        // detection_id (none)
                )

                
            }

            

        }
    }
}