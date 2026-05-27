import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Vehicle status card component
Rectangle {
    id: root

    property int vehicleId: 0
    property string vehicleName: ""
    property bool isLeader: false
    property color statusColor: "#9E9E9E"
    property double batteryPercent: 0
    property double signalStrength: 0
    property bool isArmed: false
    property bool isFlying: false

    color: isLeader ? Qt.rgba(0.13, 0.59, 1.0, 0.2) : qgcPal.panel
    radius: 4
    border.width: isLeader ? 2 : 1
    border.color: isLeader ? "#2196F3" : qgcPal.mapMission

    readonly property list<color> vehicleColors: [
        "#E91E63", "#9C27B0", "#673AB7", "#3F51B5", "#2196F3",
        "#00BCD4", "#009688", "#4CAF50", "#8BC34A", "#CDDC39",
        "#FFC107", "#FF9800", "#FF5722", "#795548", "#607D8B"
    ]

    readonly property color vehicleColor: vehicleColors[vehicleId % vehicleColors.length]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 2

        // Header row
        RowLayout {
            spacing: 4

            // Color indicator
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: vehicleColor
            }

            // Vehicle ID and name
            ColumnLayout {
                spacing: 0

                RowLayout {
                    spacing: 4

                    Label {
                        text: "UAV-%1".arg(vehicleId)
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.85
                            bold: true
                        }
                        color: qgcPal.windowText
                    }

                    Label {
                        text: "★"
                        visible: isLeader
                        font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                        color: "#FFC107"
                    }
                }

                Label {
                    text: vehicleName
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: qgcPal.windowText
                    opacity: 0.7
                }
            }

            Item { Layout.fillWidth: true }

            // Status indicator
            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: statusColor
            }
        }

        // Status row
        RowLayout {
            spacing: 8

            // Armed status
            Label {
                text: isArmed ? "ARMED" : "DISARMED"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    bold: true
                }
                color: isArmed ? "#4CAF50" : "#9E9E9E"
            }

            // Flying status
            Label {
                text: isFlying ? "FLYING" : "GROUND"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    bold: true
                }
                color: isFlying ? "#2196F3" : "#9E9E9E"
            }
        }

        // Progress bars
        ColumnLayout {
            spacing: 4

            // Battery bar
            RowLayout {
                spacing: 4

                Label {
                    text: "🔋"
                    font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                }

                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    value: batteryPercent / 100.0
                    palette {
                        trough: qgcPal.mapMission
                        highlight: batteryPercent > 30 ? "#4CAF50" : batteryPercent > 15 ? "#FF9800" : "#F44336"
                    }
                }

                Label {
                    text: "%1%".arg(batteryPercent.toFixed(0))
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: batteryPercent > 30 ? qgcPal.windowText : "#F44336"
                }
            }

            // Signal bar
            RowLayout {
                spacing: 4

                Label {
                    text: "📶"
                    font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                }

                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    value: signalStrength / 100.0
                    palette {
                        trough: qgcPal.mapMission
                        highlight: signalStrength > 60 ? "#4CAF50" : signalStrength > 30 ? "#FF9800" : "#F44336"
                    }
                }

                Label {
                    text: "%1%".arg(signalStrength.toFixed(0))
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: signalStrength > 60 ? qgcPal.windowText : "#F44336"
                }
            }
        }

        // Action buttons
        RowLayout {
            spacing: 4

            QGCButton {
                text: "Select"
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                Layout.fillWidth: true
                onClicked: {
                    SwarmManager.selectVehicle(vehicleId)
                }
            }

            QGCButton {
                text: "RTL"
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                enabled: isArmed
                onClicked: {
                    var vehicle = SwarmManager.getVehicleById(vehicleId)
                    if (vehicle) vehicle.rtl()
                }
            }
        }
    }
}