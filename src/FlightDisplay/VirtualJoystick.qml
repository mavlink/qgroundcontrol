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
    property int _seq: 0

    Timer {
        interval:   40  // 25Hz, same as real joystick rate
        running:    _virtualJoystick && _virtualJoystick.value === 1 && _activeVehicle
        repeat:     true
        onTriggered: {
            if (_activeVehicle && !_gimbalMode) {
                _activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
            }
        }
    }
    Timer {
           id: timer
       }

       function delay(delayTime, cb) {
           timer.interval = delayTime;
           timer.repeat = false;
           timer.triggered.connect(cb);
           timer.start();
       }

    function setGimbal(s) {

        //-- Set Pitch and Yaw ( -1.00 -> 1.00)
        if(s >= _seq){
            _activeVehicle.gimbalControlValue(rightStick.yAxis, rightStick.xAxis)

        }
    }

    JoystickThumbPad {
        id:                     leftStick
        anchors.leftMargin:     0
        anchors.bottomMargin:   0
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 _gimbalMode ? parent.height * 1.5 : parent.height
        yAxisThrottle:          true
        gimbalMode:             _gimbalMode
        lightColors:            useLightColors
        onYAxisChanged: {
            if(_gimbalMode) {
                //-- Set Camera Zoom ( 0.00 -> 1.00)
                _activeVehicle.cameraZoomValue(leftStick.yAxis)
            }
        }
    }

    JoystickThumbPad {
        id:                     rightStick
        anchors.rightMargin:    0
        anchors.bottomMargin:   0
        anchors.right:          parent.right
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 parent.height
        gimbalMode:             _gimbalMode
        lightColors:            useLightColors
        onYAxisChanged: {
            if(_gimbalMode) {
                _seq = _seq + 1
                var myseq = _seq
                delay(100, function() {
                    setGimbal(myseq)
                });

            }
        }
        onXAxisChanged: {
            if(_gimbalMode) {
                    _seq = _seq + 1
                    var myseq = _seq
                    delay(100, function() {
                        setGimbal(myseq)
                    });
            }
        }
    }
}
