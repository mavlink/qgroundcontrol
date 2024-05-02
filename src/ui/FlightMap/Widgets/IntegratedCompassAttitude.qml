/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap

Item {
    id:             control
    implicitWidth:  (compassRadius * 2) + attitudeSpacing + attitudeSize
    implicitHeight: implicitWidth

    property real attitudeSize:         rollIndicator.attitudeSize
    property real attitudeSpacing:      rollIndicator.attitudeSpacing
    property real extraInset:           attitudeSize + attitudeSpacing
    property real extraValuesWidth:     compassRadius
    property real defaultCompassRadius: (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:     ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:        Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:        ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:              globals.activeVehicle
    property var  qgcPal:               QGroundControl.globalPalette

    property real _totalAttitudeSize: attitudeSize + attitudeSpacing

    IntegratedAttitudeIndicator {
        id:                     rollIndicator
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   vehicle ? vehicle.roll.rawValue : 0
        compassRadius:          control.compassRadius
    }

    IntegratedAttitudeIndicator {
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   vehicle ? vehicle.pitch.rawValue : 0
        compassRadius:          control.compassRadius
        transformOrigin:        Item.Center
        rotation:               90
    }

    Rectangle {
        y:      _totalAttitudeSize
        width:  compassRadius * 2
        height: width
        radius: width / 2
        color:  qgcPal.window

        QGCCompassWidget {
            size:               parent.width - compassBorder
            vehicle:            globals.activeVehicle
            anchors.centerIn:   parent
        }
    }
}