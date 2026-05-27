import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Main swarm control panel with synchronized commands
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    readonly property real buttonHeight: ScreenTools.defaultFontPixelHeight * 2
    readonly property real buttonSpacing: ScreenTools.defaultFontPixelHeight * 0.5

    RowLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.3
        spacing: buttonSpacing

        // Formation selector
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.15
            spacing: 2

            Label {
                text: "Formation"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: qgcPal.windowText
            }

            SwarmFormationSelector {
                id: formationSelector
                Layout.fillWidth: true
            }
        }

        // Separator
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: qgcPal.mapMission
        }

        // Synchronized commands
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.6
            spacing: 2

            Label {
                text: "Swarm Commands"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: qgcPal.windowText
            }

            RowLayout {
                spacing: buttonSpacing

                // Takeoff button
                QGCButton {
                    id: takeoffButton
                    text: "🚀 Takeoff"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2.5
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.synchronizedTakeoff(20)
                    }
                }

                // Land button
                QGCButton {
                    id: landButton
                    text: "🛬 Land"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.synchronizedLand()
                    }
                }

                // RTL button
                QGCButton {
                    id: rtlButton
                    text: "🏠 RTL"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.synchronizedRTL()
                    }
                }

                // Hold button
                QGCButton {
                    id: holdButton
                    text: "⏸ Hold"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.holdPosition()
                    }
                }

                // Resume missions
                QGCButton {
                    id: resumeButton
                    text: "▶ Resume"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2.5
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.resumeAllMissions()
                    }
                }
            }
        }

        // Separator
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: qgcPal.mapMission
        }

        // Emergency controls
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.2
            spacing: 2

            Label {
                text: "Emergency"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: qgcPal.windowText
            }

            RowLayout {
                spacing: buttonSpacing

                // Emergency stop
                QGCButton {
                    id: emergencyStopButton
                    text: "⚠ STOP ALL"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2.5
                    palette.button: "red"
                    onClicked: {
                        SwarmManager.emergencyStopAll()
                    }
                }

                // Return all to home
                QGCButton {
                    id: returnHomeButton
                    text: "🏠 Return All"
                    Layout.preferredHeight: buttonHeight
                    Layout.preferredWidth: buttonHeight * 2.5
                    enabled: SwarmManager.swarmEnabled && !SwarmManager.emergencyStopActive
                    onClicked: {
                        SwarmManager.returnAllToHome()
                    }
                }
            }
        }

        // Separator
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: qgcPal.mapMission
        }

        // Swarm settings
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.25
            spacing: 2

            Label {
                text: "Swarm Settings"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    bold: true
                }
                color: qgcPal.windowText
            }

            RowLayout {
                spacing: buttonSpacing

                // Enable/disable toggle
                Switch {
                    id: swarmEnabledSwitch
                    checked: SwarmManager.swarmEnabled
                    onCheckedChanged: {
                        SwarmManager.setSwarmEnabled(checked)
                    }
                }

                Label {
                    text: "Swarm Mode"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    }
                    color: qgcPal.windowText
                    verticalAlignment: Text.AlignVCenter
                }

                // Spacing control
                Label {
                    text: "Spacing:"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                    }
                    color: qgcPal.windowText
                    verticalAlignment: Text.AlignVCenter
                }

                QGCLabel {
                    text: "%1 m".arg(SwarmManager.formationSpacing.toFixed(0))
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                    }
                    color: qgcPal.windowText
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}