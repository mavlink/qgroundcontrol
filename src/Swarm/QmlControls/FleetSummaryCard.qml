import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Fleet Summary Card showing overall swarm status at a glance
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.2
        spacing: ScreenTools.defaultFontPixelHeight * 0.1

        // Title
        Label {
            text: "Fleet Summary"
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                bold: true
            }
            color: qgcPal.windowText
        }

        // Stats row
        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight * 0.3

            // Total vehicles
            Rectangle {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                color: qgcPal.mapMission
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2

                    Label {
                        text: "✈"
                        font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        horizontalAlignment: Text.AlignHCenter
                        color: qgcPal.windowText
                    }

                    Label {
                        text: SwarmManager.totalVehicles
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                            bold: true
                        }
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }
                }
            }

            // Active vehicles
            Rectangle {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                color: SwarmManager.activeVehicles > 0 ? "#4CAF50" : "#9E9E9E"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2

                    Label {
                        text: "✓"
                        font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }

                    Label {
                        text: SwarmManager.activeVehicles
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                            bold: true
                        }
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }
                }
            }

            // Ready status
            Rectangle {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                color: SwarmManager.allVehiclesReady ? "#4CAF50" : "#FF9800"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2

                    Label {
                        text: "⚡"
                        font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }

                    Label {
                        text: SwarmManager.allVehiclesReady ? "✓" : "—"
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                            bold: true
                        }
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }
                }
            }

            // Formation status
            Rectangle {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                color: SwarmManager.currentFormation !== SwarmFormation.None ? "#2196F3" : "#9E9E9E"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2

                    Label {
                        text: "▤"
                        font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }

                    Label {
                        text: SwarmManager.currentFormation !== SwarmFormation.None ? "✓" : "—"
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                            bold: true
                        }
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                    }
                }
            }
        }

        // Battery and signal row
        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight * 0.2

            // Mini battery indicator
            RowLayout {
                spacing: 2

                Label {
                    text: "🔋"
                    font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                    color: qgcPal.windowText
                }

                Label {
                    text: "%1%".arg(Math.round(SwarmManager.getAverageBatteryLevel()))
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: SwarmManager.getAverageBatteryLevel() > 30 ? "#4CAF50" : "#FF9800"
                }
            }

            // Mini signal indicator
            RowLayout {
                spacing: 2

                Label {
                    text: "📶"
                    font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                    color: qgcPal.windowText
                }

                Label {
                    text: "%1%".arg(Math.round(SwarmManager.getMinSignalStrength()))
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: SwarmManager.getMinSignalStrength() > 60 ? "#4CAF50" : 
                           SwarmManager.getMinSignalStrength() > 30 ? "#FF9800" : "#F44336"
                }
            }

            Item { Layout.fillWidth: true }

            // Emergency indicator
            Rectangle {
                visible: SwarmManager.emergencyStopActive
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.8
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.8
                color: "#F44336"
                radius: width / 2

                Label {
                    anchors.centerIn: parent
                    text: "!"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                        bold: true
                    }
                    color: "white"
                }
            }
        }
    }
}