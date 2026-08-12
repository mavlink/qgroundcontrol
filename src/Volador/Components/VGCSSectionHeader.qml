/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Section Header
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: sectionHeaderRoot

    property string workspaceTitle: "FLIGHT OPERATIONS"
    property string workspaceSubtitle: "LIVE VEHICLE CONTROL AND TELEMETRY"

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    height: 48
    color: "#0A0F14"
    border.color: "#2C3847"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // Left: Workspace Title & Subtitle
        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false
            Layout.preferredWidth: implicitWidth
            spacing: 1

            Text {
                text: sectionHeaderRoot.workspaceTitle
                font.family: "Inter"
                font.pixelSize: 13
                font.weight: Font.Bold
                font.letterSpacing: 0.8
                color: "#F5F7FA"
                elide: Text.ElideRight
            }

            Text {
                text: sectionHeaderRoot.workspaceSubtitle
                font.family: "Inter"
                font.pixelSize: 10
                font.weight: Font.Normal
                color: "#9BA8B5"
                elide: Text.ElideRight
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: 8
        }

        // Right: Real Live Telemetry Status Chips
        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 8

            // 1. Vehicle Connection State Chip
            Rectangle {
                Layout.preferredHeight: 28
                Layout.preferredWidth: implicitWidth
                implicitWidth: connRow.implicitWidth + 18
                implicitHeight: 28
                radius: 4
                color: "#151C24"
                border.color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#2C3847")
                border.width: 1

                Row {
                    id: connRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                    }

                    Text {
                        text: isVehicleConnected ? ("VEHICLE #" + activeVehicle.id) : (isCommLost ? ("VEHICLE #" + activeVehicle.id + " • LOST") : "DISCONNECTED")
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: isVehicleConnected ? "#F5F7FA" : (isCommLost ? "#F44336" : "#9BA8B5")
                    }
                }
            }

            // 2. Flight Mode Badge
            Rectangle {
                Layout.preferredHeight: 28
                Layout.preferredWidth: implicitWidth
                implicitWidth: modeText.implicitWidth + 18
                implicitHeight: 28
                radius: 4
                color: isVehicleConnected && activeVehicle.armed ? Qt.rgba(1.0, 0.416, 0.0, 0.15) : "#151C24"
                border.color: isVehicleConnected && activeVehicle.armed ? "#FF6A00" : "#2C3847"
                border.width: 1
                visible: isVehicleConnected

                Text {
                    id: modeText
                    anchors.centerIn: parent
                    text: (activeVehicle && activeVehicle.flightMode) ? activeVehicle.flightMode.toUpperCase() : "STANDBY"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: activeVehicle && activeVehicle.armed ? "#FF6A00" : "#F5F7FA"
                }
            }

            // 3. GPS Summary Chip
            Rectangle {
                Layout.preferredHeight: 28
                Layout.preferredWidth: implicitWidth
                implicitWidth: gpsRow.implicitWidth + 18
                implicitHeight: 28
                radius: 4
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1
                visible: !!activeVehicle && !!activeVehicle.gps

                Row {
                    id: gpsRow
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        text: "GPS"
                        font.family: "Inter"
                        font.pixelSize: 9
                        font.weight: Font.DemiBold
                        color: "#9BA8B5"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count) ? (activeVehicle.gps.count.value + " SAT") : "NO FIX"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value >= 6) ? "#00C853" : "#FFC107"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // 4. Battery Summary Chip
            Rectangle {
                Layout.preferredHeight: 28
                Layout.preferredWidth: implicitWidth
                implicitWidth: batRow.implicitWidth + 18
                implicitHeight: 28
                radius: 4
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1
                visible: !!activeVehicle && !!activeVehicle.battery

                Row {
                    id: batRow
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        text: "BAT"
                        font.family: "Inter"
                        font.pixelSize: 9
                        font.weight: Font.DemiBold
                        color: "#9BA8B5"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: (activeVehicle && activeVehicle.battery && activeVehicle.battery.percentRemaining) ? (activeVehicle.battery.percentRemaining.value + "%") : "N/A"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        anchors.verticalCenter: parent.verticalCenter
                        color: {
                            if (!activeVehicle || !activeVehicle.battery || !activeVehicle.battery.percentRemaining) return "#9BA8B5"
                            var pct = activeVehicle.battery.percentRemaining.value
                            if (pct > 30) return "#00C853"
                            if (pct > 15) return "#FFC107"
                            return "#F44336"
                        }
                    }
                }
            }
        }
    }
}
