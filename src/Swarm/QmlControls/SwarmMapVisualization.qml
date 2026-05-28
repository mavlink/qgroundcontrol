import QtQuick
import QtQuick.Canvas

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Map visualization for swarm vehicles
Canvas {
    id: root

    readonly property list<color> vehicleColors: [
        "#E91E63", "#9C27B0", "#673AB7", "#3F51B5", "#2196F3",
        "#00BCD4", "#009688", "#4CAF50", "#8BC34A", "#CDDC39",
        "#FFC107", "#FF9800", "#FF5722", "#795548", "#607D8B"
    ]

    // Map viewport settings
    property real viewportScale: 1000.0 // meters per pixel at default zoom
    property var centerCoordinate: SwarmManager.swarmCenter

    // Draw function
    onPaint: function(event) {
        var ctx = getContext("2d")
        var w = width
        var h = height

        // Clear
        ctx.clearRect(0, 0, w, h)

        // Draw background
        ctx.fillStyle = qgcPal.mapBackground
        ctx.fillRect(0, 0, w, h)

        // Draw coordinate grid
        ctx.strokeStyle = qgcPal.mapMission
        ctx.lineWidth = 0.5
        ctx.globalAlpha = 0.3

        var gridStep = 100 // meters
        var centerX = w / 2
        var centerY = h / 2

        // Vertical lines
        for (var x = 0; x < w; x += gridStep * viewportScale / 1000) {
            ctx.beginPath()
            ctx.moveTo(x, 0)
            ctx.lineTo(x, h)
            ctx.stroke()
        }

        // Horizontal lines
        for (var y = 0; y < h; y += gridStep * viewportScale / 1000) {
            ctx.beginPath()
            ctx.moveTo(0, y)
            ctx.lineTo(w, y)
            ctx.stroke()
        }

        ctx.globalAlpha = 1.0

        // Draw compass rose
        drawCompassRose(ctx, w - 60, 60, 40)

        // Draw scale bar
        drawScaleBar(ctx, w - 100, h - 30)

        // Draw formation visualization
        if (SwarmManager.currentFormation !== SwarmFormation.None && SwarmManager.totalVehicles > 1) {
            drawFormation(ctx, centerX, centerY)
        }

        // Draw all vehicle positions
        var members = SwarmManager.swarmMembers
        for (var i = 0; i < members.length; i++) {
            var member = members[i]
            var vehicleX = centerX + (member.longitude - centerCoordinate.longitude) * viewportScale / 111320
            var vehicleY = centerY - (member.latitude - centerCoordinate.latitude) * viewportScale / 111320

            // Clamp to viewport
            vehicleX = Math.max(20, Math.min(w - 20, vehicleX))
            vehicleY = Math.max(20, Math.min(h - 20, vehicleY))

            drawVehicle(ctx, vehicleX, vehicleY, member, i)
        }

        // Draw center indicator
        ctx.fillStyle = "#2196F3"
        ctx.globalAlpha = 0.3
        ctx.beginPath()
        ctx.arc(centerX, centerY, 15, 0, 2 * Math.PI)
        ctx.fill()
        ctx.globalAlpha = 1.0
        ctx.strokeStyle = "#2196F3"
        ctx.lineWidth = 2
        ctx.setLineDash([5, 5])
        ctx.beginPath()
        ctx.arc(centerX, centerY, 15, 0, 2 * Math.PI)
        ctx.stroke()
        ctx.setLineDash([])
    }

    function drawVehicle(ctx, x, y, member, index) {
        var color = vehicleColors[member.id % vehicleColors.length]
        var radius = member.isLeader ? 14 : 10

        // Draw shadow
        ctx.fillStyle = "rgba(0,0,0,0.2)"
        ctx.beginPath()
        ctx.ellipse(x + 2, y + 2, radius, radius * 0.6, 0, 0, 2 * Math.PI)
        ctx.fill()

        // Draw vehicle body
        ctx.fillStyle = color
        ctx.beginPath()
        ctx.arc(x, y, radius, 0, 2 * Math.PI)
        ctx.fill()

        // Draw border
        ctx.strokeStyle = member.isLeader ? "#FFC107" : "white"
        ctx.lineWidth = member.isLeader ? 3 : 2
        ctx.stroke()

        // Draw direction indicator
        ctx.fillStyle = "white"
        ctx.beginPath()
        ctx.arc(x, y - radius - 3, 4, 0, 2 * Math.PI)
        ctx.fill()

        // Draw ID label
        ctx.fillStyle = "white"
        ctx.font = "bold %1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.6)
        ctx.textAlign = "center"
        ctx.fillText("U%1".arg(member.id), x, y + radius + 12)

        // Draw leader crown
        if (member.isLeader) {
            ctx.fillStyle = "#FFC107"
            ctx.font = "%1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.8)
            ctx.fillText("★", x, y - radius - 8)
        }

        // Draw status indicator
        var statusRadius = 5
        var statusColor = "#4CAF50"
        if (member.flying) {
            statusColor = "#2196F3"
        }

        ctx.fillStyle = statusColor
        ctx.beginPath()
        ctx.arc(x + radius - 2, y - radius + 2, statusRadius, 0, 2 * Math.PI)
        ctx.fill()
        ctx.strokeStyle = "white"
        ctx.lineWidth = 1
        ctx.stroke()
    }

    function drawCompassRose(ctx, x, y, size) {
        ctx.save()
        ctx.translate(x, y)

        // Outer circle
        ctx.strokeStyle = qgcPal.mapMission
        ctx.lineWidth = 2
        ctx.beginPath()
        ctx.arc(0, 0, size, 0, 2 * Math.PI)
        ctx.stroke()

        // North arrow
        ctx.fillStyle = "#F44336"
        ctx.beginPath()
        ctx.moveTo(0, -size)
        ctx.lineTo(-size * 0.2, 0)
        ctx.lineTo(0, -size * 0.3)
        ctx.lineTo(size * 0.2, 0)
        ctx.closePath()
        ctx.fill()

        // South arrow
        ctx.fillStyle = qgcPal.windowText
        ctx.beginPath()
        ctx.moveTo(0, size)
        ctx.lineTo(-size * 0.2, 0)
        ctx.lineTo(0, size * 0.3)
        ctx.lineTo(size * 0.2, 0)
        ctx.closePath()
        ctx.fill()

        // Labels
        ctx.fillStyle = qgcPal.windowText
        ctx.font = "bold %1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.5)
        ctx.textAlign = "center"
        ctx.fillText("N", 0, -size - 5)
        ctx.fillText("S", 0, size + 12)

        ctx.restore()
    }

    function drawScaleBar(ctx, x, y) {
        var barWidth = 50
        var barHeight = 5

        ctx.fillStyle = qgcPal.windowText
        ctx.fillRect(x, y, barWidth, barHeight)

        ctx.font = "%1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.4)
        ctx.textAlign = "center"
        ctx.fillText("100m", x + barWidth / 2, y - 5)
    }

    function drawFormation(ctx, centerX, centerY) {
        var members = SwarmManager.swarmMembers
        if (members.length < 2) return

        ctx.strokeStyle = "#2196F3"
        ctx.lineWidth = 2
        ctx.globalAlpha = 0.5
        ctx.setLineDash([10, 5])

        ctx.beginPath()
        var first = true
        for (var i = 0; i < members.length; i++) {
            var member = members[i]
            var x = centerX + (member.longitude - centerCoordinate.longitude) * viewportScale / 111320
            var y = centerY - (member.latitude - centerCoordinate.latitude) * viewportScale / 111320

            if (first) {
                ctx.moveTo(x, y)
                first = false
            } else {
                ctx.lineTo(x, y)
            }
        }
        ctx.closePath()
        ctx.stroke()

        ctx.globalAlpha = 1.0
        ctx.setLineDash([])
    }

    // Update on changes
    Connections {
        target: SwarmManager

        function onSwarmMembersChanged() {
            root.requestPaint()
        }

        function onSwarmCenterChanged() {
            root.requestPaint()
        }

        function onFormationUpdateRequired() {
            root.requestPaint()
        }
    }
}