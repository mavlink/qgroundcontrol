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

    property alias attitudeSize:                rollIndicator.attitudeSize
    property alias attitudeSpacing:             rollIndicator.attitudeSpacing
    property real extraInset:                   attitudeSize + attitudeSpacing
    property real extraValuesWidth:             compassRadius
    property real defaultCompassRadius:         (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:             ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:                Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:                ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:                      globals.activeVehicle
    property var  qgcPal:                       QGroundControl.globalPalette
    property bool usedByMultipleVehicleList:    false
    property color compassFaceColor:            Qt.rgba(0.105, 0.110, 0.116, 0.58)
    property color compassShellColor:           Qt.rgba(0.86, 0.88, 0.90, 0.10)
    property color compassArcColor:             Qt.rgba(0.86, 0.88, 0.90, 0.18)
    property color compassTickColor:            Qt.rgba(0.86, 0.88, 0.90, 0.82)
    property color headingPrimaryColor:         Qt.rgba(0.88, 0.91, 0.94, 0.96)
    property color headingSecondaryColor:       Qt.rgba(0.54, 0.57, 0.60, 0.96)

    property real _totalAttitudeSize: attitudeSize + attitudeSpacing

    IntegratedAttitudeIndicator {
        id:                     rollIndicator
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   vehicle ? vehicle.roll.rawValue : 0
        compassRadius:          control.compassRadius
        arcBackgroundColor:     control.compassArcColor
        arcValueColor:          qgcPal.primaryButton
        tickColor:              control.compassTickColor
    }

    IntegratedAttitudeIndicator {
        x:                      -_totalAttitudeSize
        attitudeAngleDegrees:   vehicle ? vehicle.pitch.rawValue : 0
        compassRadius:          control.compassRadius
        attitudeSize:           control.attitudeSize
        attitudeSpacing:        control.attitudeSpacing
        arcBackgroundColor:     control.compassArcColor
        arcValueColor:          qgcPal.primaryButton
        tickColor:              control.compassTickColor
        transformOrigin:        Item.Center
        rotation:               90
    }

    Rectangle {
        y:      _totalAttitudeSize
        width:  compassRadius * 2
        height: width
        radius: width / 2
        color:  control.compassShellColor

        QGCCompassWidget {
            size:                       parent.width - compassBorder
            vehicle:                    control.vehicle
            usedByMultipleVehicleList:  control.usedByMultipleVehicleList
            compassFaceColor:           control.compassFaceColor
            compassBorderColor:         "transparent"
            compassTickColor:           control.compassTickColor
            compassLabelColor:          qgcPal.text
            headingPrimaryColor:        control.headingPrimaryColor
            headingSecondaryColor:      control.headingSecondaryColor
            headingStrokeColor:         Qt.rgba(0.94, 0.98, 1.0, 0.72)
            anchors.centerIn:           parent
        }
    }
}
