/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Sub-Header Action Toolbar
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import VoladorTheme 1.0

Rectangle {
    id: toolbarRoot

    implicitHeight: 44
    color: ThemeController.isDark ? "#161D27" : "#F1F5F9"
    border.color: ThemeController.border
    border.width: 1

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        // Active Vehicle Selector Badge
        Rectangle {
            implicitHeight: 28
            implicitWidth: 160
            radius: 6
            color: ThemeController.isDark ? "#202834" : "#E2E8F0"
            border.color: isVehicleConnected ? ThemeController.accent : (isCommLost ? ThemeController.danger : ThemeController.border)

            RowLayout {
                anchors.centerIn: parent
                spacing: 6

                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: isVehicleConnected ? ThemeController.success : (isCommLost ? ThemeController.danger : ThemeController.textSecondary)
                }

                Text {
                    text: isVehicleConnected ? ("DRONE #" + activeVehicle.id) : (isCommLost ? ("DRONE #" + activeVehicle.id + " • LOST") : "NO VEHICLE")
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: ThemeController.textPrimary
                }
            }
        }

        Rectangle { implicitWidth: 1; implicitHeight: 18; color: ThemeController.border }

        // Telemetry Quick Chips
        RowLayout {
            spacing: 8

            // GPS Status Pill
            Rectangle {
                implicitHeight: 26; implicitWidth: 96; radius: 6
                color: ThemeController.isDark ? "#202834" : "#FFFFFF"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "📡"; font.pixelSize: 11 }
                    Text {
                        text: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " Sats") : (isCommLost ? "NO FIX (LOST)" : "NO FIX")
                        font.family: "JetBrains Mono"; font.pixelSize: 10; font.weight: Font.Bold
                        color: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? ThemeController.success : (isCommLost ? ThemeController.danger : (isVehicleConnected ? ThemeController.accent : ThemeController.textSecondary))
                    }
                }
            }

            // Battery Status Pill
            Rectangle {
                implicitHeight: 26; implicitWidth: 100; radius: 6
                color: ThemeController.isDark ? "#202834" : "#FFFFFF"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "🔋"; font.pixelSize: 11 }
                    Text {
                        text: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                        font.family: "JetBrains Mono"; font.pixelSize: 10; font.weight: Font.Bold
                        color: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? ThemeController.success : (isCommLost ? ThemeController.danger : ThemeController.textSecondary)
                    }
                }
            }

            // Mission Time Pill
            Rectangle {
                implicitHeight: 26; implicitWidth: 90; radius: 6
                color: ThemeController.isDark ? "#202834" : "#FFFFFF"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "⏱️"; font.pixelSize: 11 }
                    Text {
                        text: "00:24:18"
                        font.family: "JetBrains Mono"; font.pixelSize: 10; font.weight: Font.Bold
                        color: ThemeController.accent
                    }
                }
            }
        }

        Item { Layout.fillWidth: true } // Center Spacer

        // Live Clock & System Status
        RowLayout {
            spacing: 12

            Rectangle {
                implicitHeight: 24; implicitWidth: 130; radius: 12
                color: ThemeController.isDark ? "#1C2532" : "#E2E8F0"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Rectangle { width: 6; height: 6; radius: 3; color: ThemeController.success }
                    Text {
                        text: "SYS OPERATIONAL"
                        font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold
                        color: ThemeController.textPrimary
                    }
                }
            }

            Text {
                id: clockText
                font.family: "JetBrains Mono"
                font.pixelSize: 11
                font.weight: Font.Bold
                color: ThemeController.textPrimary
                text: Qt.formatTime(new Date(), "hh:mm:ss UTC")

                Timer {
                    interval: 1000; running: true; repeat: true
                    onTriggered: clockText.text = Qt.formatTime(new Date(), "hh:mm:ss UTC")
                }
            }
        }
    }
}
