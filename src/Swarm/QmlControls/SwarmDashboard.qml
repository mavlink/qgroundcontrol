import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// @brief Fleet summary card showing overall swarm status
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.3
        spacing: ScreenTools.defaultFontPixelHeight * 0.2

        // Title
        Label {
            text: "Fleet Overview"
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                bold: true
            }
            color: qgcPal.windowText
        }

        // Stats grid
        GridLayout {
            columns: 2
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.3
            columnSpacing: ScreenTools.defaultFontPixelHeight * 0.5

            // Total vehicles
            StatItem {
                label: "Total"
                value: SwarmManager.totalVehicles
                icon: "✈"
            }

            // Active vehicles
            StatItem {
                label: "Active"
                value: SwarmManager.activeVehicles
                icon: "✓"
                valueColor: SwarmManager.activeVehicles > 0 ? "#4CAF50" : "#9E9E9E"
            }

            // Ready vehicles
            StatItem {
                label: "Ready"
                value: SwarmManager.allVehiclesReady ? SwarmManager.totalVehicles : "—"
                icon: "⚡"
            }

            // Formation status
            StatItem {
                label: "Formation"
                value: SwarmManager.currentFormation !== SwarmFormation.None ? "✓" : "—"
                icon: "▤"
            }
        }

        // Battery status
        RowLayout {
            spacing: ScreenTools.defaultFontPixelHeight * 0.3

            Label {
                text: "Avg Battery:"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                }
                color: qgcPal.windowText
            }

            ProgressBar {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 6
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                value: SwarmManager.getAverageBatteryLevel() / 100.0
                palette {
                    trough: qgcPal.mapMission
                    highlight: SwarmManager.getAverageBatteryLevel() > 30 ? "#4CAF50" : "#FF9800"
                }
            }

            Label {
                text: "%1%".arg(SwarmManager.getAverageBatteryLevel().toFixed(0))
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: SwarmManager.getAverageBatteryLevel() > 30 ? "#4CAF50" : "#FF9800"
            }
        }

        // Signal status
        RowLayout {
            spacing: ScreenTools.defaultFontPixelHeight * 0.3

            Label {
                text: "Signal:"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                }
                color: qgcPal.windowText
            }

            // Signal strength indicator
            Row {
                spacing: 2

                Repeater {
                    model: 5
                    delegate: Rectangle {
                        width: ScreenTools.defaultFontPixelHeight * 0.5
                        height: ScreenTools.defaultFontPixelHeight * 0.5 + index * 3
                        radius: 2
                        color: {
                            var strength = SwarmManager.getMinSignalStrength()
                            if (strength >= (index + 1) * 20) {
                                return strength > 60 ? "#4CAF50" : strength > 30 ? "#FF9800" : "#F44336"
                            }
                            return qgcPal.mapMission
                        }
                    }
                }
            }

            Label {
                text: "%1%".arg(SwarmManager.getMinSignalStrength().toFixed(0))
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: SwarmManager.getMinSignalStrength() > 60 ? "#4CAF50" : 
                       SwarmManager.getMinSignalStrength() > 30 ? "#FF9800" : "#F44336"
            }
        }
    }

    // Stat item component
    component StatItem: RowLayout {
        spacing: 4

        Label {
            text: icon
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
            }
            color: qgcPal.windowText
        }

        Label {
            text: label + ":"
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
            }
            color: qgcPal.windowText
        }

        Label {
            text: value
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                bold: true
            }
            color: valueColor || qgcPal.windowText
        }

        property string label: ""
        property var value: ""
        property string icon: ""
        property color valueColor: qgcPal.windowText
    }
}