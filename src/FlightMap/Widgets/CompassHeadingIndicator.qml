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




Canvas {
    id:                 control
    anchors.centerIn:   parent
    width:              compassSize * 1/3
    height:             width

    property real compassSize
    property real heading
    property bool simplified:    false

    property var _qgcPal: QGroundControl.globalPalette

    Connections {
        target:                 _qgcPal
        function onGlobalThemeChanged() { control.requestPaint() }
    }

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        ctx.lineWidth = 1.6
        ctx.lineJoin = 'round'
        ctx.lineCap = 'round'

        // Arrow path (right side)
        ctx.beginPath()
        ctx.moveTo(width / 2, 0)
        ctx.lineTo(width, height)
        ctx.lineTo(width / 2, height * 0.75)
        ctx.closePath()

        // Gradient fill inside triangle (no external shadow box)
        var gradR = ctx.createRadialGradient(width/2, height*0.28, 0, width/2, height*0.28, height*0.65)
        gradR.addColorStop(0, _qgcPal.brandingBlue)
        gradR.addColorStop(1, '#00000000')
        ctx.fillStyle = gradR
        ctx.fill()

        // Left side
        ctx.beginPath()
        ctx.moveTo(width / 2, 0)
        ctx.lineTo(0, height)
        ctx.lineTo(width / 2, height * 0.75)
        ctx.closePath()

        var gradL = ctx.createRadialGradient(width/2, height*0.28, 0, width/2, height*0.28, height*0.65)
        gradL.addColorStop(0, _qgcPal.brandingBlue)
        gradL.addColorStop(1, '#00000000')
        ctx.fillStyle = gradL
        ctx.fill()

        // Crisp outline using brand color
        ctx.strokeStyle = _qgcPal.brandingBlue
        ctx.beginPath()
        ctx.moveTo(width / 2, 0)
        ctx.lineTo(width, height)
        ctx.lineTo(width / 2, height * 0.75)
        ctx.closePath()
        ctx.stroke()

        ctx.beginPath()
        ctx.moveTo(width / 2, 0)
        ctx.lineTo(0, height)
        ctx.lineTo(width / 2, height * 0.75)
        ctx.closePath()
        ctx.stroke()
    }

    transform: Rotation {
        origin.x:   control.width / 2
        origin.y:   control.height / 2
        angle:      heading
    }
}
