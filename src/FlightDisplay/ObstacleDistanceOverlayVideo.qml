/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                          2.11

import QGroundControl                   1.0
import QGroundControl.SettingsManager   1.0

Item {
    id: root
    anchors.fill: parent
    property var showText: obstacleDistance._showText

    function drawSegment(ctx, range, centerX, centerY, lengthFrom, lengthTo, radFrom, radTo, grad) {
        const topSrcX = centerX + lengthFrom * Math.cos(radFrom)
        const topSrcY = centerY + lengthFrom * Math.sin(radFrom)
        const topDstX = centerX + lengthFrom * Math.cos(radTo)
        const topDstY = centerY + lengthFrom * Math.sin(radTo)

        ctx.save()
        ctx.setTransform(2, 0, 0, 1, -centerX, 0)
        ctx.beginPath()
        ctx.fillStyle = grad
        ctx.moveTo(topSrcX, topSrcY)
        ctx.arc(centerX, centerY, lengthFrom, radFrom, radTo, false)
        ctx.arc(centerX, centerY, lengthTo, radTo, radFrom, true)
        ctx.lineTo(topSrcX, topSrcY)
        ctx.fill()
        ctx.closePath()
        ctx.restore()

        if (range > 0) {
            ctx.save()
            ctx.setTransform(2, 0, 0, 2, -centerX, 0)
            ctx.beginPath()
            ctx.strokeStyle = Qt.rgba(0, 0, 0, 0.8)
            ctx.font = "bold 10px sans-serif"
            ctx.lineWidth = 2
            ctx.fillStyle = Qt.rgba(1, 1, 1, 0.8)
            const text = obstacleDistance._rangeToShow(range)
            const textX = topSrcX + (topDstX - topSrcX) / 2
            const textY = topSrcY + (topDstY - topSrcY) / 2
            ctx.strokeText(text, textX, textY / 2)
            ctx.fillText(text, textX, textY / 2)
            ctx.closePath()
            ctx.restore()
        }
    }

    function paintObstacleOverlay(ctx) {
        const centerX = root.width / 2
        const centerY = root.height / 2
        const maxRadiusPixels = 0.9 * root.height / 2 // Max pixels to center
        const minRadiusPixels = maxRadiusPixels * 0.2 // Min radius in pixels to center
        const minGradPixels = minRadiusPixels
        const maxGradPixels = maxRadiusPixels
        const segmentHeightPixels = minRadiusPixels / 8
        const levelMeters = 10
        const levelNum = obstacleDistance._maxRadiusMeters / levelMeters

        var grad = ctx.createRadialGradient(centerX, centerY, maxGradPixels - segmentHeightPixels * levelNum * 2, centerX, centerY, maxGradPixels)
        grad.addColorStop(0, Qt.rgba(1, 0, 0, 0.9))
        grad.addColorStop(0.1, Qt.rgba(1, 0, 0, 0.3))
        grad.addColorStop(0.5, Qt.rgba(1, 0.64, 0, 0.3))
        grad.addColorStop(0.65, Qt.rgba(1, 0.64, 0, 0.2))
        grad.addColorStop(0.95, Qt.rgba(0, 1, 0, 0.1))
        grad.addColorStop(1, Qt.rgba(0, 1, 0, 0))

        const segNum = 16
        const incDeg = 360 / segNum
        for (var s = 0; s < segNum; ++s) {
            const deg = s * 360.0 / segNum
            const rad = deg * Math.PI / 180.0
            const i = obstacleDistance._degToRangeIdx(deg, false)
            const iNext = obstacleDistance._degToRangeIdx(deg + incDeg, false)
            var rangeMin = obstacleDistance._maxRadiusMeters

            const end = i < iNext ? iNext : obstacleDistance._rangesLen + iNext
            for (var ii = i; ii < end; ++ii) {
                const r = obstacleDistance._ranges[ii % obstacleDistance._rangesLen] / 100
                if (r < rangeMin)
                    rangeMin = r
            }

            const lengthFrom = maxRadiusPixels
            const radFrom = rad
            const radTo = radFrom + incDeg * Math.PI / 180.0 - 0.03
            var range = obstacleDistance._maxRadiusMeters
            for (var ii = 0; ii < levelNum; ++ii) {
                const from = lengthFrom - ii * segmentHeightPixels * 2
                const to = from - segmentHeightPixels
                const rangeInLevel = obstacleDistance._maxRadiusMeters - (ii + 1) * levelMeters
                const isLast = ii >= levelNum - 1
                if (rangeMin > rangeInLevel || isLast)
                    range = rangeMin
                const rangeToShow = showText && range < obstacleDistance._maxRadiusMeters ? range : 0
                drawSegment(ctx, rangeToShow, centerX, centerY, from, to, radFrom, radTo, grad)
                if (range == rangeMin)
                    break
            }
        }
    }

    ObstacleDistanceOverlay {
        id: obstacleDistance
    }
}
