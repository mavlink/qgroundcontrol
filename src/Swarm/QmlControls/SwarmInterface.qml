import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import Swarm

/// @brief Professional Swarm Interface Dashboard
/// Provides centralized swarm control, monitoring, and coordination
Item {
    id: root

    // Constants for styling
    readonly property real panelMargins: ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real panelSpacing: ScreenTools.defaultFontPixelHeight * 0.3
    readonly property real iconSize: ScreenTools.defaultFontPixelHeight * 1.5

    // Swarm status colors
    readonly property color statusReady: "#4CAF50"
    readonly property color statusInMission: "#2196F3"
    readonly property color statusWarning: "#FF9800"
    readonly property color statusError: "#F44336"
    readonly property color statusDisconnected: "#9E9E9E"

    // Vehicle colors for map display
    readonly property list<color> vehicleColors: [
        "#E91E63", "#9C27B0", "#673AB7", "#3F51B5", "#2196F3",
        "#00BCD4", "#009688", "#4CAF50", "#8BC34A", "#CDDC39",
        "#FFC107", "#FF9800", "#FF5722", "#795548", "#607D8B"
    ]

    Connections {
        target: SwarmManager

        function onSwarmEnabledChanged(enabled) {
            swarmEnabledIndicator.visible = enabled
            statusText.text = enabled ? "SWARM ACTIVE" : "SWARM DISABLED"
        }

        function onEmergencyStopActiveChanged(active) {
            if (active) {
                emergencyOverlay.visible = true
                emergencyOverlay.opacity = 1.0
            } else {
                emergencyOverlay.opacity = 0.0
                emergencyOverlay.visible = false
            }
        }

        function onSwarmStatusTextChanged(status) {
            statusText.text = status
        }
    }

    // Main layout
    Rectangle {
        id: mainBackground
        anchors.fill: parent
        color: qgcPal.window

        // Emergency stop overlay
        Rectangle {
            id: emergencyOverlay
            visible: false
            anchors.fill: parent
            color: Qt.rgba(0.96, 0.27, 0.27, emergencyOverlay.opacity)
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: 200 } }

            Label {
                anchors.centerIn: parent
                text: "⚠ EMERGENCY STOP ACTIVE ⚠"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 3
                    bold: true
                }
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    SwarmManager.resumeFromEmergency()
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: panelMargins
            spacing: panelSpacing

            // Top toolbar with swarm controls
            SwarmControlPanel {
                id: controlPanel
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 4
            }

            // Main content area split between map and vehicle list
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: panelSpacing

                // Left panel: Fleet Overview
                ColumnLayout {
                    Layout.preferredWidth: parent.width * 0.25
                    Layout.fillHeight: true
                    spacing: panelSpacing

                    // Fleet summary card
                    FleetSummaryCard {
                        id: fleetSummary
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                    }

                    // Vehicle list
                    SwarmVehicleList {
                        id: vehicleList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }

                // Center: Map with all vehicles
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: qgcPal.mapBackground
                    border.width: 1
                    border.color: qgcPal.mapMission
                    radius: 4

                    // Map placeholder - integrate with actual FlightMap
                    SwarmMapVisualization {
                        id: swarmMap
                        anchors.fill: parent
                    }
                }

                // Right panel: Telemetry and Alerts
                ColumnLayout {
                    Layout.preferredWidth: parent.width * 0.2
                    Layout.fillHeight: true
                    spacing: panelSpacing

                    // Swarm health indicator
                    SwarmHealthIndicator {
                        id: healthIndicator
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                    }

                    // Telemetry charts
                    SwarmTelemetryWidget {
                        id: telemetryWidget
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    // Alert system
                    SwarmAlertSystem {
                        id: alertSystem
                        Layout.fillWidth: true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 10
                    }
                }
            }

            // Bottom status bar
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                spacing: panelSpacing

                // Status indicator
                Rectangle {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                    Layout.fillHeight: true
                    color: SwarmManager.swarmEnabled ? statusReady : statusDisconnected
                    radius: 4

                    Label {
                        anchors.centerIn: parent
                        text: SwarmManager.swarmEnabled ? "●" : "○"
                        font.pixelSize: ScreenTools.defaultFontPixelHeight
                        color: "white"
                    }
                }

                // Status text
                Label {
                    id: statusText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: SwarmManager.swarmStatusText
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                        bold: true
                    }
                    verticalAlignment: Text.AlignVCenter
                    color: qgcPal.windowText
                }

                // Vehicle count
                Label {
                    Layout.preferredWidth: implicitWidth
                    text: "Vehicles: %1/%2".arg(SwarmManager.activeVehicles).arg(SwarmManager.totalVehicles)
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                    }
                    color: qgcPal.windowText
                }

                // Formation mode
                Label {
                    Layout.preferredWidth: implicitWidth
                    text: "Formation: %1".arg(SwarmManager.currentFormation)
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                    }
                    color: qgcPal.windowText
                }
            }
        }

        // Swarm enabled indicator (floating)
        Rectangle {
            id: swarmEnabledIndicator
            visible: SwarmManager.swarmEnabled
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: panelMargins
            width: labelWidth + paddingWidth * 2
            height: ScreenTools.defaultFontPixelHeight * 1.5
            color: statusReady
            radius: 4

            readonly property real paddingWidth: ScreenTools.defaultFontPixelHeight * 0.5
            readonly property real labelWidth: statusLabel.implicitWidth

            Label {
                id: statusLabel
                anchors.centerIn: parent
                text: "SWARM MODE"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: "white"
            }
        }
    }

    // Component definitions
    Component {
        id: vehicleColorDelegate

        Rectangle {
            width: ScreenTools.defaultFontPixelHeight * 0.8
            height: width
            radius: width / 2
            color: root.vehicleColors[index % root.vehicleColors.length]
        }
    }
}