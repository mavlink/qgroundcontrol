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

    function paintObstacleOverlay(ctx) {
        const vehiclePoint = _root.fromCoordinate(_activeVehicleCoordinate, false)
        const centerX = vehiclePoint.x
        const centerY = vehiclePoint.y
        const maxRadiusPixels = 0.9 * root.height / 2 // Max pixels to center
        const minRadiusPixels = maxRadiusPixels * 0.2
        const metersPerPixelInCycle = (maxRadiusPixels - minRadiusPixels) / obstacleDistance._maxRadiusMeters

        const leftCoord  = mapControl.toCoordinate(Qt.point(0, root.y), false)
        const rightCoord = mapControl.toCoordinate(Qt.point(100, root.y), false)
        const metersIn100Pixels = leftCoord.distanceTo(rightCoord)
        const metersPerPixel = 100.0 / metersIn100Pixels

        var minGradPixels = minRadiusPixels
        var maxGradPixels = maxRadiusPixels
        var metersToPixels = metersPerPixelInCycle
        if (metersIn100Pixels < 4) {
            minGradPixels = 0
            maxGradPixels =  obstacleDistance._maxRadiusMeters * metersPerPixel
            metersToPixels = metersPerPixel
        }

        var grad = ctx.createRadialGradient(centerX, centerY, minGradPixels, centerX, centerY, maxGradPixels)
        grad.addColorStop(0, Qt.rgba(1, 0, 0, 1))
        grad.addColorStop(0.1, Qt.rgba(1, 0, 0, 0.7))
        grad.addColorStop(0.5, Qt.rgba(1, 0.64, 0, 0.7))
        grad.addColorStop(0.65, Qt.rgba(1, 0.64, 0, 0.3))
        grad.addColorStop(0.95, Qt.rgba(0, 1, 0, 0.3))
        grad.addColorStop(1, Qt.rgba(0, 1, 0, 0))

        var points = []
        const height = minRadiusPixels / 8
        for (var i = 0; i < obstacleDistance._rangesLen; ++i) {
            const deg = i * obstacleDistance._incrementDeg
            const rad =  deg * Math.PI / 180.0
            const m = obstacleDistance._ranges[obstacleDistance._degToRangeIdx(deg, true)] / 100.0
            const pixels = minGradPixels + m * metersToPixels
            const outerX = centerX + pixels * Math.cos(rad)
            const outerY = centerY + pixels * Math.sin(rad)
            const innerX = centerX + (pixels - height) * Math.cos(rad)
            const innerY = centerY + (pixels - height) * Math.sin(rad)

            points.push({'outer_x': outerX, 'outer_y': outerY, 'inner_x': innerX, 'inner_y': innerY, 'range': m})
        }

        ctx.strokeStyle = Qt.rgba(0, 0, 0, 0.8)
        ctx.font = "bold 22px sans-serif"
        ctx.lineWidth = 2;
        var mPrev = -1
        for (var i = 0; i < points.length; i += 3) {
            const i3 = (i + 3) % points.length // catch the line from the last to the first point

            ctx.beginPath()
            ctx.fillStyle = grad
            ctx.moveTo(points[i].inner_x, points[i].inner_y)
            ctx.lineTo(points[i].outer_x, points[i].outer_y)
            ctx.bezierCurveTo(
                points[i + 1].outer_x, points[i + 1].outer_y,
                points[i + 2].outer_x, points[i + 2].outer_y,
                points[i3].outer_x, points[i3].outer_y)
            ctx.lineTo(points[i3].inner_x, points[i3].inner_y)
            ctx.bezierCurveTo(
                points[i3].inner_x, points[i3].inner_y,
                points[i + 2].inner_x, points[i + 2].inner_y,
                points[i + 1].inner_x, points[i + 1].inner_y)
            ctx.fill()

            if (showText) {
                var iMin = i
                for (var k = iMin + 1; k < iMin + 2; ++k) {
                    const idx = k % points.length
                    if (points[idx].range < points[iMin].range)
                        iMin = idx
                }

                var m = points[iMin].range
                if (m < obstacleDistance._maxRadiusMeters && Math.abs(m - mPrev) > 2.0) {
                    const textX = points[iMin].inner_x
                    const textY = points[iMin].inner_y

                    ctx.fillStyle = Qt.rgba(1, 1, 1, 0.9)
                    const text = obstacleDistance._rangeToShow(m)
                    ctx.strokeText(text, textX, textY)
                    ctx.fillText(text, textX, textY)
                    mPrev = m
                }
            }
        }
    }

    ObstacleDistanceOverlay {
        id: obstacleDistance
    }
}

