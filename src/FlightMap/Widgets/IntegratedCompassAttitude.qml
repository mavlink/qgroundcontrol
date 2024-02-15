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
    width:  totalRadius * 2
    height: width

    property real totalRadius:          compassRadius + attitudeSpacing + attitudeSize
    property real attitudeSize:         rollIndicator.attitudeSize
    property real attitudeSpacing:      rollIndicator.attitudeSpacing
    property real defaultCompassRadius: (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:     ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:        Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:        ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:              globals.activeVehicle
    property var  qgcPal:               QGroundControl.globalPalette

    IntegratedAttitudeIndicator {
        id:                     rollIndicator
        anchors.fill:           parent
        attitudeAngleDegrees:   vehicle ? vehicle.roll.rawValue : 0
    }

    IntegratedAttitudeIndicator {
        anchors.fill:           parent
        attitudeAngleDegrees:   vehicle ? vehicle.pitch.rawValue : 0
        transformOrigin:        Item.Center
        rotation:               90
    }
    Rectangle {
        anchors.centerIn:   parent
        width:              compassRadius * 2
        height:             width
        radius:             width / 2
        color:              qgcPal.window

        DeadMouseArea { anchors.fill: parent }

        QGCCompassWidget {
            size:               parent.width - compassBorder
            vehicle:            globals.activeVehicle
            anchors.centerIn:   parent
        }
    }
}