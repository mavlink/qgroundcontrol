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
    implicitWidth:  _totalRadius * 2
    implicitHeight: implicitWidth

    property real compassRadius:        ScreenTools.defaultFontPixelHeight * 6 / 2
    property real attitudeAngleDegrees: 0

    readonly property real attitudeSize:         ScreenTools.defaultFontPixelHeight * 0.75
    readonly property real attitudeSpacing:      ScreenTools.defaultFontPixelHeight / 4

    property real _totalRadius:             compassRadius + attitudeSpacing + attitudeSize
    property real _attitudeRadius:          (width / 2) - (attitudeSize / 2)
    property real _maxAngleDegrees:         30
    property real _maxRadians:              _maxAngleDegrees * Math.PI / 180
    property real _zeroAttitudeRadians:     Math.PI * 1.5
    property real _clampedAngleDegrees:     Math.min(Math.max(attitudeAngleDegrees, -_maxAngleDegrees), _maxAngleDegrees)
    property real _attitudeAnglePercent:    _clampedAngleDegrees / _maxAngleDegrees

    property var qgcPal:  QGroundControl.globalPalette

    on_AttitudeAnglePercentChanged: angleIndicator.requestPaint()

    // Roll background
    Canvas {
        anchors.fill: parent

        onPaint: {
            var centerX = width / 2
            var centerY = height / 2
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.window
            ctx.lineWidth = attitudeSize
            ctx.beginPath()
            ctx.arc(centerX, centerY, _attitudeRadius, _zeroAttitudeRadians - _maxRadians, _zeroAttitudeRadians + _maxRadians)
            ctx.stroke()
        }
    }

    // Roll value indicator
    Canvas {
        id:             angleIndicator
        anchors.fill:   parent
        visible:        Math.abs(attitudeAngleDegrees) > 1

        property real startRollRadiansRaw:      _zeroAttitudeRadians
        property real endRollRadiansRaw:        _zeroAttitudeRadians + (_attitudeAnglePercent * _maxRadians)
        property real startRollRadiansOrdered:  Math.min(startRollRadiansRaw, endRollRadiansRaw)
        property real endRollRadiansOrdered:    Math.max(startRollRadiansRaw, endRollRadiansRaw)

        onPaint: {
            var centerX = width / 2
            var centerY = height / 2
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.primaryButton
            ctx.lineWidth = attitudeSize
            ctx.beginPath()
            ctx.arc(centerX, centerY, _attitudeRadius, startRollRadiansOrdered, endRollRadiansOrdered)
            ctx.stroke()
        }
    }

    // Roll 0 value tick mark
    Canvas {
        anchors.fill: parent

        onPaint: {
            var centerX = width / 2
            var centerY = height / 2
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = qgcPal.text
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(centerX, 0)
            ctx.lineTo(centerX, attitudeSize)
            ctx.stroke()
        }
    }
}