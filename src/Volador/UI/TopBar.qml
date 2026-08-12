/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Command TopBar
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Palette
import VoladorTheme 1.0

Rectangle {
    id: topBarRoot

    height: 54
    color: ThemeController.topbar
    border.color: ThemeController.border
    border.width: 1

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost
    property string missionTime: "00:24:18"
    property string weatherInfo: "24°C ☀️ 8 km/h NW"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 14

        // 1. Logo & App Title
        RowLayout {
            spacing: 8
            Image {
                source: "qrc:/Volador/Assets/Logos/volador_compact.png"
                implicitWidth: 32
                implicitHeight: 32
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                mipmap: true
            }

            Column {
                Layout.alignment: Qt.AlignVCenter
                Text {
                    text: "VGCS"
                    font.family: "Inter"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: ThemeController.textPrimary
                }
                Text {
                    text: "COMMAND CENTER"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    color: ThemeController.accent
                }
            }
        }

        Rectangle { width: 1; height: 24; color: ThemeController.border }

        // 2. Active Vehicle Selector Chip
        Rectangle {
            height: 34
            implicitWidth: 170
            radius: 6
            color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
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
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: ThemeController.textPrimary
                }
            }
        }

        // 3. Rounded Search Bar
        Rectangle {
            height: 34
            Layout.preferredWidth: 180
            radius: 6
            color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
            border.color: searchInput.activeFocus ? ThemeController.accent : ThemeController.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6
                Text { text: "🔍"; font.pixelSize: 12 }
                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    font.family: "Inter"
                    font.pixelSize: 12
                    color: ThemeController.textPrimary
                    Text {
                        text: "Search telemetry..."
                        font.family: "Inter"
                        font.pixelSize: 12
                        color: ThemeController.textSecondary
                        visible: !searchInput.text && !searchInput.activeFocus
                    }
                }
            }
        }

        Item { Layout.fillWidth: true } // Center Spacer

        // 4. Telemetry Chips (GPS, Battery, Mission Time, Signal)
        RowLayout {
            spacing: 8

            // GPS Status
            Rectangle {
                height: 32; implicitWidth: 100; radius: 6
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "📡"; font.pixelSize: 12 }
                    Text {
                        text: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " Sats") : (isCommLost ? "NO FIX (LOST)" : "NO FIX")
                        font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                        color: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? ThemeController.success : (isCommLost ? ThemeController.danger : (isVehicleConnected ? ThemeController.accent : ThemeController.textSecondary))
                    }
                }
            }

            // Battery Status
            Rectangle {
                height: 32; implicitWidth: 100; radius: 6
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "🔋"; font.pixelSize: 12 }
                    Text {
                        text: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                        font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                        color: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? ThemeController.success : (isCommLost ? ThemeController.danger : ThemeController.textSecondary)
                    }
                }
            }

            // Mission Time
            Rectangle {
                height: 32; implicitWidth: 100; radius: 6
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "⏱️"; font.pixelSize: 12 }
                    Text {
                        text: topBarRoot.missionTime
                        font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                        color: ThemeController.accent
                    }
                }
            }

            // Weather Placeholder
            Rectangle {
                height: 32; implicitWidth: 130; radius: 6
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text {
                        text: topBarRoot.weatherInfo
                        font.family: "Inter"; font.pixelSize: 11
                        color: ThemeController.textSecondary
                    }
                }
            }
        }

        Rectangle { width: 1; height: 24; color: ThemeController.border }

        // 5. Action Icons & Profile (Theme Toggle, Notifications, Profile, Clock)
        RowLayout {
            spacing: 8

            // Theme Toggle Button
            Rectangle {
                width: 34; height: 34; radius: 17
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                border.color: ThemeController.border
                Text {
                    anchors.centerIn: parent
                    text: ThemeController.isDark ? "☀️" : "🌙"
                    font.pixelSize: 14
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: ThemeController.toggleTheme()
                }
            }

            // Notifications Bell
            Rectangle {
                width: 34; height: 34; radius: 17
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                border.color: ThemeController.border
                Text { anchors.centerIn: parent; text: "🔔"; font.pixelSize: 14 }
                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: ThemeController.accent
                    anchors.top: parent.top; anchors.right: parent.right
                }
            }

            // User Profile Chip
            Rectangle {
                height: 34; implicitWidth: 110; radius: 17
                color: ThemeController.isDark ? "#20242A" : "#F3F5F7"
                border.color: ThemeController.border
                RowLayout {
                    anchors.centerIn: parent; spacing: 6
                    Rectangle {
                        width: 24; height: 24; radius: 12; color: ThemeController.accent
                        Text { anchors.centerIn: parent; text: "👨‍✈️"; font.pixelSize: 12 }
                    }
                    Text {
                        text: "PILOT"
                        font.family: "Inter"; font.pixelSize: 11; font.weight: Font.Bold
                        color: ThemeController.textPrimary
                    }
                }
            }

            // Live Clock
            Text {
                id: liveClock
                font.family: "JetBrains Mono"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: ThemeController.textPrimary
                text: Qt.formatTime(new Date(), "hh:mm:ss")
                Timer {
                    interval: 1000; running: true; repeat: true
                    onTriggered: liveClock.text = Qt.formatTime(new Date(), "hh:mm:ss")
                }
            }
        }
    }
}
