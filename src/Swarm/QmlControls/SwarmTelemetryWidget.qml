import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Telemetry data visualization widget
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    property var telemetryData: SwarmManager.getSwarmHealthStatus()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Title
        Label {
            text: "Swarm Telemetry"
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                bold: true
            }
            color: qgcPal.windowText
        }

        // Battery chart placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 4
            color: qgcPal.mapBackground
            radius: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4

                Label {
                    text: "Battery Levels"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    }
                    color: qgcPal.windowText
                }

                // Mini bar chart
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 2

                    Repeater {
                        model: SwarmManager.swarmMembers.length > 0 ? SwarmManager.swarmMembers.length : 5

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            readonly property var member: SwarmManager.swarmMembers[index]
                            readonly property double battery: member ? member.batteryPercent : 0

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width - 2
                                height: parent.height * (battery / 100.0)
                                anchors.horizontalCenter: parent.horizontalCenter
                                radius: 2
                                color: battery > 30 ? "#4CAF50" : battery > 15 ? "#FF9800" : "#F44336"
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width - 2
                                height: parent.height
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: "transparent"
                                border.width: 1
                                border.color: qgcPal.mapMission
                                radius: 2
                            }

                            Label {
                                anchors.bottom: parent.bottom
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: index + 1
                                font {
                                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                                }
                                color: qgcPal.windowText
                            }
                        }
                    }
                }
            }
        }

        // Signal strength chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 2
            color: qgcPal.mapBackground
            radius: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4

                Label {
                    text: "Signal Strength"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    }
                    color: qgcPal.windowText
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Repeater {
                        model: 5

                        delegate: Column {
                            Layout.fillWidth: true
                            spacing: 2

                            Rectangle {
                                width: parent ? parent.width : 10
                                height: ScreenTools.defaultFontPixelHeight * 0.6
                                color: index < (SwarmManager.getMinSignalStrength() / 20) ? "#4CAF50" : qgcPal.mapMission
                                radius: 2
                            }

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: index + 1
                                font {
                                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.4
                                }
                                color: qgcPal.windowText
                            }
                        }
                    }
                }
            }
        }

        // Stats summary
        GridLayout {
            columns: 3
            rows: 2
            Layout.fillWidth: true
            spacing: 4

            StatBox {
                statLabel: "Ready"
                statValue: telemetryData.readyVehicles || 0
                statColor: "#4CAF50"
            }

            StatBox {
                statLabel: "Flying"
                statValue: telemetryData.flyingVehicles || 0
                statColor: "#2196F3"
            }

            StatBox {
                statLabel: "Battery"
                statValue: "%1%".arg(telemetryData.averageBattery ? telemetryData.averageBattery.toFixed(0) : "0")
                statColor: "#8BC34A"
            }
        }
    }

    // Stat box component
    component StatBox: Rectangle {
        id: statBox
        color: qgcPal.mapBackground
        radius: 2

        property string statLabel: ""
        property string statValue: ""
        property color statColor: qgcPal.windowText

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 2

            Label {
                text: statLabel
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                }
                color: qgcPal.windowText
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                text: statValue
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                    bold: true
                }
                color: statBox.statColor
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}