/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Vehicle Card
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: cardRoot

    property var vehicle: null
    readonly property bool isCommLost: vehicle !== null && vehicle.vehicleLinkManager !== null && vehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: vehicle !== null && vehicle.vehicleLinkManager !== null && !vehicle.vehicleLinkManager.communicationLost
    readonly property bool isActive: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager && QGroundControl.multiVehicleManager.activeVehicle === vehicle)

    signal inspectRequested(var vehicle)

    implicitWidth: 360
    implicitHeight: mainCol.implicitHeight + 28
    radius: 4
    color: cardMouse.containsMouse ? "#1D2733" : "#151C24"
    border.color: isActive ? "#FF6A00" : (cardMouse.containsMouse ? "#3A4B5E" : "#2C3847")
    border.width: isActive ? 2 : 1

    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 14
        spacing: 10

        // 1. Header: Vehicle ID, Type, and Active State Badge
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                width: 8
                height: 8
                radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: isVehicleConnected ? (vehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    text: vehicle ? ("DRONE #" + vehicle.id) : "NO VEHICLE"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: {
                        if (!vehicle) return "DISCONNECTED"
                        if (vehicle.vehicleTypeString && vehicle.vehicleTypeString.length > 0) return vehicle.vehicleTypeString
                        if (vehicle.px4Firmware) return "PX4 Pro Multirotor"
                        if (vehicle.apmFirmware) return "ArduPilot UAS"
                        return "Generic UAS"
                    }
                    font.family: "Inter"
                    font.pixelSize: 9
                    color: "#9BA8B5"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // Active Vehicle Status Badge
            Rectangle {
                Layout.preferredHeight: 22
                Layout.preferredWidth: activeLabel.implicitWidth + 14
                implicitWidth: Layout.preferredWidth
                implicitHeight: 22
                radius: 3
                color: isActive ? Qt.rgba(1.0, 0.416, 0.0, 0.2) : "#1D2733"
                border.color: isActive ? "#FF6A00" : "#2C3847"
                border.width: 1

                Text {
                    id: activeLabel
                    anchors.centerIn: parent
                    text: isActive ? "ACTIVE VEHICLE" : "STANDBY"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    color: isActive ? "#FF6A00" : "#9BA8B5"
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // 2. Telemetry & State Metric Rows
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 8
            columnSpacing: 12

            // Metric A: Flight Mode & Arm State
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: "FLIGHT MODE"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Text {
                    text: vehicle ? (vehicle.flightMode ? vehicle.flightMode.toUpperCase() : "STANDBY") : "N/A"
                    font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                    color: (vehicle && vehicle.armed) ? "#FF6A00" : "#F5F7FA"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: vehicle ? (vehicle.armed ? "ARMED • LIVE" : "DISARMED • SAFE") : "DISCONNECTED"
                    font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold
                    color: vehicle ? (vehicle.armed ? "#FF6A00" : "#00C853") : "#64748B"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // Metric B: GNSS Positioning
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: "GNSS / SATELLITES"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Text {
                    text: (vehicle && vehicle.gps && vehicle.gps.count && vehicle.gps.count.value !== undefined && !isNaN(vehicle.gps.count.value) && vehicle.gps.count.value > 0) ? (vehicle.gps.count.value + " SATS") : "NO FIX"
                    font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                    color: (vehicle && vehicle.gps && vehicle.gps.count && vehicle.gps.count.value !== undefined && vehicle.gps.count.value >= 6) ? "#00C853" : (vehicle ? "#FFC107" : "#64748B")
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: (vehicle && vehicle.gps && vehicle.gps.hdop && vehicle.gps.hdop.value !== undefined && !isNaN(vehicle.gps.hdop.value)) ? ("HDOP: " + vehicle.gps.hdop.value.toFixed(2)) : "N/A"
                    font.family: "JetBrains Mono"; font.pixelSize: 8; color: (vehicle && vehicle.gps && vehicle.gps.hdop && vehicle.gps.hdop.value !== undefined && !isNaN(vehicle.gps.hdop.value)) ? "#9BA8B5" : "#64748B"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // Metric C: Battery & Power
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: "POWER SYSTEM"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Text {
                    text: (vehicle && vehicle.battery && vehicle.battery.percentRemaining && vehicle.battery.percentRemaining.value !== undefined && !isNaN(vehicle.battery.percentRemaining.value) && vehicle.battery.percentRemaining.value >= 0) ? (Math.round(vehicle.battery.percentRemaining.value) + "%") : "N/A"
                    font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                    color: {
                        if (!vehicle || !vehicle.battery || !vehicle.battery.percentRemaining || vehicle.battery.percentRemaining.value === undefined || isNaN(vehicle.battery.percentRemaining.value) || vehicle.battery.percentRemaining.value < 0) return "#64748B"
                        var pct = vehicle.battery.percentRemaining.value
                        if (pct > 30) return "#00C853"
                        if (pct > 15) return "#FFC107"
                        return "#F44336"
                    }
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: (vehicle && vehicle.battery && vehicle.battery.voltage && vehicle.battery.voltage.value !== undefined && !isNaN(vehicle.battery.voltage.value) && vehicle.battery.voltage.value > 0) ? (vehicle.battery.voltage.value.toFixed(1) + " V") : "N/A"
                    font.family: "JetBrains Mono"; font.pixelSize: 8; color: (vehicle && vehicle.battery && vehicle.battery.voltage && vehicle.battery.voltage.value !== undefined && !isNaN(vehicle.battery.voltage.value) && vehicle.battery.voltage.value > 0) ? "#9BA8B5" : "#64748B"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // Metric D: Communication & MAVLink Link
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: "COMMUNICATION"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Text {
                    text: isVehicleConnected ? "MAVLINK v2.0" : (isCommLost ? "LINK LOST" : "OFFLINE")
                    font.family: "JetBrains Mono"; font.pixelSize: 11; font.weight: Font.Bold
                    color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: {
                        if (!isVehicleConnected) return "N/A"
                        if (typeof vehicle.telemetryLRSSI !== "undefined" && vehicle.telemetryLRSSI !== 0) {
                            return vehicle.telemetryLRSSI + " dBm"
                        }
                        if (typeof vehicle.rcRSSI !== "undefined" && vehicle.rcRSSI > 0 && vehicle.rcRSSI <= 100) {
                            return vehicle.rcRSSI + "%"
                        }
                        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.primaryLinkName) {
                            return vehicle.vehicleLinkManager.primaryLinkName
                        }
                        return "N/A"
                    }
                    font.family: "JetBrains Mono"; font.pixelSize: 8
                    color: (isVehicleConnected && ((typeof vehicle.telemetryLRSSI !== "undefined" && vehicle.telemetryLRSSI !== 0) || (typeof vehicle.rcRSSI !== "undefined" && vehicle.rcRSSI > 0 && vehicle.rcRSSI <= 100))) ? "#9BA8B5" : "#64748B"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // 3. Selection & Inspection Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Select Active Control Button
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                height: 28
                radius: 3
                color: isActive ? "#1D2733" : (selectBtnMouse.containsMouse ? "#FF6A00" : "#2C3847")
                border.color: isActive ? "#FF6A00" : "#2C3847"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: isActive ? "✓ CONTROLLING" : "SELECT CONTROL"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: isActive ? "#FF6A00" : (selectBtnMouse.containsMouse ? "#0A0F14" : "#F5F7FA")
                }

                MouseArea {
                    id: selectBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager && vehicle) {
                            QGroundControl.multiVehicleManager.activeVehicle = vehicle
                        }
                    }
                }
            }

            // Inspect Details Button
            Rectangle {
                Layout.preferredHeight: 28
                Layout.preferredWidth: inspectText.implicitWidth + 16
                implicitWidth: Layout.preferredWidth
                height: 28
                radius: 3
                color: inspectMouse.containsMouse ? "#2C3847" : "#1D2733"
                border.color: inspectMouse.containsMouse ? "#FF6A00" : "#2C3847"
                border.width: 1

                Text {
                    id: inspectText
                    anchors.centerIn: parent
                    text: "DETAILS ➔"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }

                MouseArea {
                    id: inspectMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (vehicle) {
                            if (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) {
                                QGroundControl.multiVehicleManager.activeVehicle = vehicle
                            }
                            cardRoot.inspectRequested(vehicle)
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        z: -1
        onClicked: {
            if (vehicle) {
                if (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) {
                    QGroundControl.multiVehicleManager.activeVehicle = vehicle
                }
                cardRoot.inspectRequested(vehicle)
            }
        }
    }
}
