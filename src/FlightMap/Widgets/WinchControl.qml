
import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Window               2.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

import MAVLink                      1.0

Rectangle {
    id:                                     winchControlRect
    // MAVLink definitions not yet implemented in QGC.
    // Using the actions from https://mavlink.io/en/messages/common.html#WINCH_ACTIONS
    enum WinchCommands {
        WINCH_RELAXED = 0,
        WINCH_DELIVER = 4,
        WINCH_RETRACT = 6
    }

    property real   _topBottomMargin:       (width * 0.05) / 2

    Layout.fillWidth:               true
    anchors.bottom:                 parent.bottom

    function uncheckInactiveButtons() {
        winchEmergencyBtn.checked = false
        winchDeliverBtn.checked = false
        winchRetractBtn.checked = false
 
        switch(slider._currentWinchCommand) {
            case null:
                return
            case WinchControl.WinchCommands.WINCH_RELAXED:
                winchEmergencyBtn.checked = true
                break
            case WinchControl.WinchCommands.WINCH_DELIVER:
                winchDeliverBtn.checked = true
                break
            case WinchControl.WinchCommands.WINCH_RETRACT:
                winchRetractBtn.checked = true
                break
        }
    }

    QGCButton {
        id:                         winchEmergencyBtn
        backRadius:                 10
        showBorder:                 true
        width:                      parent.width
        text:                       "EMERGENCY RELEASE"
        background:                 Rectangle {
            color:                  "red"
            radius:                 10
            border.color:           "white"
            border.width:           1
        }
        checkable:                  true
        anchors.bottom:             winchDeliverBtn.top
        anchors.bottomMargin:       _topBottomMargin
        Image {
            id:                     alert
            source:                 "/qmlimages/Yield.svg"
            height:                 parent.height
            width:                  height
            x:                      10
        }
        onClicked:  winchRelax()

        function winchRelax() {
            if (checked) {
                slider.enableWinchSlider(WinchControl.WinchCommands.WINCH_RELAXED, qsTr("Emergency release winch"))
                uncheckInactiveButtons()
            } else {
                slider.disableWinchSlider()
            }
        }
    }

    QGCButton {
        id:                         winchDeliverBtn
        backRadius:                 10
        showBorder:                 true
        width:                      parent.width
        text:                       "DELIVER"
        checkable:                  true
        anchors.bottom:             winchRetractBtn.top
        anchors.bottomMargin:       _topBottomMargin
        Image {
            id:                     downArrow
            source:                 "/qmlimages/ArrowDirection.svg"
            height:                 parent.height
            width:                  height
            x:                      10
            transform: Rotation {
                origin.x:           downArrow.width/2
                origin.y:           downArrow.height/2
                angle:              180
            }
        }
        onClicked:  winchDeliver()
        
        function winchDeliver() {
            if (checked) {
                slider.enableWinchSlider(WinchControl.WinchCommands.WINCH_DELIVER, qsTr("Perform payload drop"))
                uncheckInactiveButtons()
            } else {
                slider.disableWinchSlider()
            }
        }
    }

    QGCButton {
        id:                         winchRetractBtn
        backRadius:                 10
        showBorder:                 true
        width:                      parent.width
        text:                       "RETRACT"
        checkable:                  true
        anchors.bottom:             slider.top
        anchors.bottomMargin:       _topBottomMargin
        Image {
            id:                     upArrow
            source:                 "/qmlimages/ArrowDirection.svg"
            height:                 parent.height
            width:                  height
            x:                      10
        }
        onClicked: winchRetract()
        
        function winchRetract() {
            if (checked) {
                slider.enableWinchSlider(WinchControl.WinchCommands.WINCH_RETRACT, qsTr("Perform winch retract"))
                uncheckInactiveButtons()
            } else {
                slider.disableWinchSlider()
            }
        }
    }

    SliderSwitch {
        id:                                     slider
        property var _vehicle:                  QGroundControl.multiVehicleManager.activeVehicle
        visible:                                false
        width:                                  parent.width
        confirmText:                            qsTr("Not visible")
        anchors.bottom:                         parent.bottom
        anchors.bottomMargin:                   2 * _topBottomMargin
        
        property var    _currentWinchCommand:   null
        onAccept: sendWinchCommand()
        
        function enableWinchSlider(command, message) {
            visible = true
            confirmText = message
            _currentWinchCommand = command
        }

        function disableWinchSlider() {
            visible = false
            confirmText = qsTr("Not visible")
            _currentWinchCommand = null
        }

        function sendWinchCommand() {
            if (_vehicle === null) {
                console.log("Cannot send winch command, vehicle not connected.")
            } else if (_currentWinchCommand === null) {
                console.log("Cannot send winch command, no action selected.")
            } else {
                //MAV_COMP_ID_USER18(42) is the chosen component for winches
                // since no winch defaults exist yet in the MAVLink standard.
                // The QML MAVLink enum doesn't include MAV_CMD_DO_WINCH(42600).
                // Setting it explicitly. See src/comm/QGCMAVLink.h for details.           
                _vehicle.sendCommand(42, 42600, 1, 1, _currentWinchCommand, 1, 1)
                disableWinchSlider()
                uncheckInactiveButtons()
            }
        }
    }
}


