/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.5

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

Item {
    QGCMapPalette { id: mapPal; lightColors: !isBackgroundDark }

    Timer {
        interval:   40  // 25Hz, same as real joystick rate
        running:    QGroundControl.virtualTabletJoystick && _activeVehicle
        repeat:     true
        onTriggered: {
            if (_activeVehicle) {
                _activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
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
        height:                 parent.height
        yAxisThrottle:          true
        lightColors:            !isBackgroundDark
    }

    JoystickThumbPad {
        id:                     rightStick
        anchors.rightMargin:    -xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.right:          parent.right
        anchors.bottom:         parent.bottom
        width:                  parent.height
        height:                 parent.height
        lightColors:            !isBackgroundDark
    }
}
