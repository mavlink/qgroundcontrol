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

    Item {
        width: 400
        height: 250

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
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
                var digiviewManager = QGroundControl.digiviewManager
                if (!digiviewManager) return

                var step = 10.0
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

                digiviewManager.sendCamTargetingParameters(
                    "stream",   // stream_name
                    0,        // cam_id
                    1,        // targeting_mode — euler delta mode
                    true,        // euler_delta = true
                    yaw,      // yaw
                    pitch,    // pitch
                    0,        // roll
                    0,        // lock_flags
                    0, 0,     // x_offset, y_offset
                    0, 0, 0,  // target lat/lon/alt (unused in delta mode)
                    0,        //track-id
                    -1,
                    false        // detection_id (none)
                )

                
            }

            

        }
    }
}
