/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

Item {
    //property bool useLightColors - Must be passed in from loaded
    property Fact _virtualJoystick: QGroundControl.settingsManager.appSettings.virtualJoystick
    property bool _gimbalMode:      _activeVehicle && _virtualJoystick && _virtualJoystick.value === 2

    property real _lastP:           0
    property real _lastY:           0
    property real _lastZ:           0

    function getFrequency() {
        if(!_activeVehicle) {
            return 0
        }
        //-- Gimbal doesn't need a high frequency (4Hz)
        if(_gimbalMode) {
            return 250
        }
        // 40  - 25Hz, same as real joystick rate
        // 200 - 5Hz,  for slow links
        if(_activeVehicle && _activeVehicle.linkType === Vehicle.LinkNormal) {
            return 40
        }
        return 200
    }

    function isActive() {
        if(!_virtualJoystick) {
            return false
        }
        if(_virtualJoystick.value < 1) {
            return false
        }
        if(!_activeVehicle) {
            return false
        }
        if(_gimbalMode && !_activeVehicle.gimbalAcknowledged) {
            return false
        }
        if(_activeVehicle.linkType === Vehicle.LinkNone || _activeVehicle.linkType === Vehicle.LinkHighLatency) {
            return false
        }
        return true
    }

    Timer {
        interval:   getFrequency()
        running:    isActive()
        repeat:     true
        onTriggered: {
            if (_activeVehicle) {
                if(_gimbalMode) {
                    //-- Gimbal changes are only sent if they exist. These are
                    //   MAVLink Commands
                    if(rightStick.yAxis !== _lastP || rightStick.xAxis !== _lastY) {
                        _lastP = rightStick.yAxis
                        _lastY = rightStick.xAxis
                        //-- Set Pitch and Yaw ( -1.00 -> 1.00)
                        _activeVehicle.gimbalControlValue(rightStick.yAxis, rightStick.xAxis)
                    }
                    if(leftStick.yAxis !== _lastZ) {
                        _lastZ = leftStick.yAxis
                        //-- Set Camera Zoom ( 0.00 -> 1.00)
                        _activeVehicle.cameraZoomValue(leftStick.yAxis)
                    }
                } else {
                    //-- For joystick, we send a stream of messages just like a real RC
                    _activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
                }
            }
        }
    }

    JoystickThumbPad {
        id:                     leftStick
        anchors.leftMargin:     xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 _gimbalMode ? parent.height * 1.5 : parent.height
        yAxisThrottle:          true
        gimbalMode:             _gimbalMode
        lightColors:            useLightColors
    }

    JoystickThumbPad {
        id:                     rightStick
        anchors.rightMargin:    -xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.right:          parent.right
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 parent.height
        gimbalMode:             _gimbalMode
        lightColors:            useLightColors
    }
}
