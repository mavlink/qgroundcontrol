import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Swarm health status indicator
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    property var healthStatus: SwarmManager.getSwarmHealthStatus()

    readonly property color healthGood: "#4CAF50"
    readonly property color healthWarning: "#FF9800"
    readonly property color healthCritical: "#F44336"

    readonly property color currentHealthColor: {
        if (healthStatus.collisionRisk) return healthCritical
        if (healthStatus.emergencyActive) return healthCritical
        if (healthStatus.averageBattery < 30) return healthWarning
        return healthGood
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Title with health indicator
        RowLayout {
            spacing: 4

            // Health status circle
            Rectangle {
                width: 16
                height: 16
                radius: 8
                color: currentHealthColor

                SequentialAnimation on opacity {
                    running: healthStatus.emergencyActive
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.3; duration: 500 }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 500 }
                }
            }

            Label {
                text: "Swarm Health"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                    bold: true
                }
                color: qgcPal.windowText
            }

            Item { Layout.fillWidth: true }

            Label {
                text: healthStatusText
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: currentHealthColor
            }
        }

        // Health metrics grid
        GridLayout {
            columns: 2
            rows: 3
            Layout.fillWidth: true
            columnSpacing: 4
            rowSpacing: 4

            HealthMetric {
                label: "Vehicles"
                value: "%1/%2".arg(healthStatus.totalVehicles).arg(healthStatus.activeVehicles)
                icon: "✈"
                statusColor: healthStatus.totalVehicles > 0 ? healthGood : healthCritical
            }

            HealthMetric {
                label: "Battery"
                value: "%1%".arg(healthStatus.averageBattery ? healthStatus.averageBattery.toFixed(0) : "0")
                icon: "🔋"
                statusColor: healthStatus.averageBattery > 30 ? healthGood : 
                             healthStatus.averageBattery > 15 ? healthWarning : healthCritical
            }

            HealthMetric {
                label: "Signal"
                value: "%1%".arg(healthStatus.minSignal ? healthStatus.minSignal.toFixed(0) : "0")
                icon: "📶"
                statusColor: healthStatus.minSignal > 60 ? healthGood :
                             healthStatus.minSignal > 30 ? healthWarning : healthCritical
            }

            HealthMetric {
                label: "Collision"
                value: healthStatus.collisionRisk ? "⚠" : "✓"
                icon: "⚠"
                statusColor: healthStatus.collisionRisk ? healthCritical : healthGood
            }

            HealthMetric {
                label: "Formation"
                value: healthStatus.formationLocked ? "🔒" : "○"
                icon: "▤"
                statusColor: healthStatus.formationLocked ? healthGood : healthWarning
            }

            HealthMetric {
                label: "Emergency"
                value: healthStatus.emergencyActive ? "🚨" : "✓"
                icon: "🚨"
                statusColor: healthStatus.emergencyActive ? healthCritical : healthGood
            }
        }

        // Refresh button
        QGCButton {
            text: "Refresh Health"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
            onClicked: {
                healthStatus = SwarmManager.getSwarmHealthStatus()
            }
        }
    }

    // Health metric component
    component HealthMetric: Rectangle {
        color: qgcPal.mapBackground
        radius: 2

        RowLayout {
            spacing: 4
            anchors.fill: parent
            anchors.margins: 2

            Label {
                text: icon
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                }
            }

            ColumnLayout {
                spacing: 0

                Label {
                    text: label
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                    }
                    color: qgcPal.windowText
                }

                Label {
                    text: value
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                        bold: true
                    }
                    color: statusColor
                }
            }
        }

        property string label: ""
        property string value: ""
        property string icon: ""
        property color statusColor: qgcPal.windowText
    }

    readonly property string healthStatusText: {
        if (healthStatus.emergencyActive) return "CRITICAL"
        if (healthStatus.collisionRisk) return "WARNING"
        if (healthStatus.averageBattery < 30) return "LOW BATTERY"
        return "HEALTHY"
    }
}