import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Mini map for swarm visualization
Rectangle {
    id: root

    color: qgcPal.mapBackground
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    readonly property list<color> vehicleColors: [
        "#E91E63", "#9C27B0", "#673AB7", "#3F51B5", "#2196F3",
        "#00BCD4", "#009688", "#4CAF50", "#8BC34A", "#CDDC39",
        "#FFC107", "#FF9800", "#FF5722", "#795548", "#607D8B"
    ]

    // Scale and bounds for the map view
    property real mapScale: 100.0 // pixels per meter
    property point mapCenter: Qt.point(0, 0)
    property real viewWidth: width
    property real viewHeight: height

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 2

        // Header with controls
        RowLayout {
            spacing: 4

            Label {
                text: "Swarm Map"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                    bold: true
                }
                color: qgcPal.windowText
            }

            Item { Layout.fillWidth: true }

            // Zoom controls
            QGCButton {
                text: "+"
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                onClicked: mapScale *= 1.5
            }

            QGCButton {
                text: "-"
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                onClicked: mapScale /= 1.5
            }

            QGCButton {
                text: "Center"
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                onClicked: centerOnSwarm()
            }
        }

        // Map canvas
        Canvas {
            id: mapCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Draw the map
            onPaint: function(event) {
                var ctx = getContext("2d")
                var w = width
                var h = height

                // Clear canvas
                ctx.clearRect(0, 0, w, h)

                // Draw grid
                ctx.strokeStyle = qgcPal.mapMission
                ctx.lineWidth = 0.5
                ctx.setLineDash([2, 2])

                var gridSpacing = mapScale / 10 // Grid every 10m
                for (var x = 0; x < w; x += gridSpacing) {
                    ctx.beginPath()
                    ctx.moveTo(x, 0)
                    ctx.lineTo(x, h)
                    ctx.stroke()
                }
                for (var y = 0; y < h; y += gridSpacing) {
                    ctx.beginPath()
                    ctx.moveTo(0, y)
                    ctx.lineTo(w, y)
                    ctx.stroke()
                }

                ctx.setLineDash([])

                // Draw center point
                var centerX = w / 2
                var centerY = h / 2

                ctx.fillStyle = "#2196F3"
                ctx.beginPath()
                ctx.arc(centerX, centerY, 8, 0, 2 * Math.PI)
                ctx.fill()

                // Draw swarm center label
                ctx.fillStyle = qgcPal.windowText
                ctx.font = "%1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.6)
                ctx.fillText("SWARM CENTER", centerX - 40, centerY - 15)

                // Draw formation lines
                if (SwarmManager.currentFormation !== SwarmFormation.None) {
                    ctx.strokeStyle = "#2196F3"
                    ctx.lineWidth = 2
                    ctx.setLineDash([5, 5])

                    var members = SwarmManager.swarmMembers
                    if (members.length > 1) {
                        ctx.beginPath()
                        var first = true
                        for (var i = 0; i < members.length; i++) {
                            var member = members[i]
                            var x = centerX + (member.longitude - mapCenter.x) * mapScale
                            var y = centerY + (member.latitude - mapCenter.y) * mapScale

                            // Clamp to visible area
                            x = Math.max(10, Math.min(w - 10, x))
                            y = Math.max(10, Math.min(h - 10, y))

                            if (first) {
                                ctx.moveTo(x, y)
                                first = false
                            } else {
                                ctx.lineTo(x, y)
                            }
                        }
                        ctx.stroke()
                    }

                    ctx.setLineDash([])
                }

                // Draw vehicle positions
                var members = SwarmManager.swarmMembers
                for (var i = 0; i < members.length; i++) {
                    var member = members[i]
                    var x = centerX + (member.longitude - mapCenter.x) * mapScale
                    var y = centerY + (member.latitude - mapCenter.y) * mapScale

                    // Clamp to visible area
                    x = Math.max(15, Math.min(w - 15, x))
                    y = Math.max(15, Math.min(h - 15, y))

                    // Draw vehicle circle
                    var color = vehicleColors[member.id % vehicleColors.length]
                    ctx.fillStyle = color
                    ctx.beginPath()
                    ctx.arc(x, y, member.isLeader ? 12 : 8, 0, 2 * Math.PI)
                    ctx.fill()

                    // Draw border
                    ctx.strokeStyle = "white"
                    ctx.lineWidth = 2
                    ctx.stroke()

                    // Draw direction indicator
                    ctx.fillStyle = "white"
                    ctx.beginPath()
                    ctx.arc(x, y - (member.isLeader ? 12 : 8) - 2, 3, 0, 2 * Math.PI)
                    ctx.fill()

                    // Draw label
                    ctx.fillStyle = color
                    ctx.font = "bold %1px sans-serif".arg(ScreenTools.defaultFontPixelHeight * 0.5)
                    ctx.fillText("U%1".arg(member.id), x - 10, y + (member.isLeader ? 20 : 15))
                }
            }

            // Redraw when data changes
            Connections {
                target: SwarmManager

                function onSwarmMembersChanged() {
                    mapCanvas.requestPaint()
                }

                function onSwarmCenterChanged() {
                    mapCanvas.requestPaint()
                }

                function onFormationUpdateRequired() {
                    mapCanvas.requestPaint()
                }
            }
        }

        // Legend
        RowLayout {
            spacing: 8

            Label {
                text: "Legend:"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                }
                color: qgcPal.windowText
            }

            // Leader indicator
            Row {
                spacing: 4

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: "#2196F3"
                    border.width: 2
                    border.color: "white"
                }

                Label {
                    text: "Leader"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                    }
                    color: qgcPal.windowText
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Follower indicator
            Row {
                spacing: 4

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: "#E91E63"
                    border.width: 1
                    border.color: "white"
                }

                Label {
                    text: "Follower"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                    }
                    color: qgcPal.windowText
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Function to center map on swarm
    function centerOnSwarm() {
        var center = SwarmManager.swarmCenter
        mapCenter = Qt.point(center.longitude, center.latitude)
        mapCanvas.requestPaint()
    }
}