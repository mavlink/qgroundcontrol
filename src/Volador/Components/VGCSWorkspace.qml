/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Workspace Container
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: workspaceRoot

    property string currentRoute: "flight"
    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    readonly property real headerHeight: headerColumn.height

    color: "#0A0F14"

    function setCurrentRoute(route) {
        currentRoute = route
        switch (route) {
        case "flight":
            sectionHeader.workspaceTitle = "FLIGHT OPERATIONS"
            sectionHeader.workspaceSubtitle = "LIVE VEHICLE CONTROL AND TELEMETRY"
            break
        case "missions":
            sectionHeader.workspaceTitle = "MISSION PLANNING"
            sectionHeader.workspaceSubtitle = "Survey, waypoint routing & geo-fence configuration"
            break
        case "vehicles":
            sectionHeader.workspaceTitle = "VEHICLE CONFIGURATION"
            sectionHeader.workspaceSubtitle = "Autopilot calibration, sensors, radio & parameters"
            break
        case "map":
            sectionHeader.workspaceTitle = "MAP WORKSPACE"
            sectionHeader.workspaceSubtitle = "Geospatial positioning, satellite layers & offline map sets"
            break
        case "telemetry":
            sectionHeader.workspaceTitle = "TELEMETRY & ANALYSIS"
            sectionHeader.workspaceSubtitle = "Real-time MAVLink inspector, packet logs & sensor graphs"
            break
        case "video":
            sectionHeader.workspaceTitle = "VIDEO FEEDS"
            sectionHeader.workspaceSubtitle = "Low-latency camera streaming & payload gimbal control"
            break
        case "logs":
            sectionHeader.workspaceTitle = "FLIGHT LOGS"
            sectionHeader.workspaceSubtitle = "Onboard flight data recorder & uLog downloader"
            break
        case "settings":
            sectionHeader.workspaceTitle = "APPLICATION SETTINGS"
            sectionHeader.workspaceSubtitle = "System preferences, comm links, RTK & security options"
            break
        }
    }

    Column {
        id: headerColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
        z: 20

        // 1. Workspace Header
        VGCSSectionHeader {
            id: sectionHeader
            width: parent.width
        }

        // 2. Top Telemetry & Status Cards Strip
        Rectangle {
            width: parent.width
            height: 60
            color: "#0F161E"
            border.color: "#2C3847"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                // Card 1: Vehicle
                VGCSStatusCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    categoryLabel: "VEHICLE"
                    primaryValue: isVehicleConnected ? ("DRONE #" + activeVehicle.id) : (isCommLost ? ("DRONE #" + activeVehicle.id) : "NO VEHICLE")
                    secondaryValue: isVehicleConnected ? ((activeVehicle.vehicleTypeString && activeVehicle.vehicleTypeString.length > 0) ? activeVehicle.vehicleTypeString : (activeVehicle.px4Firmware ? "PX4 Pro Multirotor" : (activeVehicle.apmFirmware ? "ArduPilot UAS" : "Generic UAS"))) : (isCommLost ? "LINK LOST" : "DISCONNECTED")
                    statusColor: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
                }

                // Card 2: Communication Link
                VGCSStatusCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    categoryLabel: "COMMUNICATION"
                    primaryValue: isVehicleConnected ? "ONLINE" : (isCommLost ? "LINK LOST" : "OFFLINE")
                    secondaryValue: {
                        if (!activeVehicle) return "NO LINK"
                        if (isCommLost) return "HEARTBEAT TIMEOUT"
                        if (typeof activeVehicle.telemetryLRSSI !== "undefined" && activeVehicle.telemetryLRSSI !== 0) {
                            return activeVehicle.telemetryLRSSI + " dBm"
                        }
                        if (typeof activeVehicle.rcRSSI !== "undefined" && activeVehicle.rcRSSI > 0 && activeVehicle.rcRSSI <= 100) {
                            return activeVehicle.rcRSSI + "% RSSI"
                        }
                        if (activeVehicle.vehicleLinkManager && activeVehicle.vehicleLinkManager.primaryLinkName) {
                            return activeVehicle.vehicleLinkManager.primaryLinkName
                        }
                        return "MAVLINK v2.0"
                    }
                    statusColor: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                }

                // Card 3: GNSS Positioning
                VGCSStatusCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    categoryLabel: "GNSS POSITION"
                    primaryValue: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " SATS") : (isCommLost ? "NO FIX (LOST)" : "NO FIX")
                    secondaryValue: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.hdop && activeVehicle.gps.hdop.value !== undefined && !isNaN(activeVehicle.gps.hdop.value)) ? ("HDOP: " + activeVehicle.gps.hdop.value.toFixed(1)) : "N/A"
                    statusColor: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? "#00C853" : (isCommLost ? "#F44336" : (isVehicleConnected ? "#FFC107" : "#64748B"))
                }

                // Card 4: Battery & Power
                VGCSStatusCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    categoryLabel: "POWER SYSTEM"
                    primaryValue: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                    secondaryValue: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.voltage && activeVehicle.battery.voltage.value !== undefined && !isNaN(activeVehicle.battery.voltage.value) && activeVehicle.battery.voltage.value > 0) ? (activeVehicle.battery.voltage.value.toFixed(1) + " V") : "N/A"
                    statusColor: {
                        if (!isVehicleConnected || !activeVehicle.battery || !activeVehicle.battery.percentRemaining || activeVehicle.battery.percentRemaining.value === undefined || isNaN(activeVehicle.battery.percentRemaining.value) || activeVehicle.battery.percentRemaining.value < 0) return isCommLost ? "#F44336" : "#64748B"
                        var pct = activeVehicle.battery.percentRemaining.value
                        if (pct > 30) return "#00C853"
                        if (pct > 15) return "#FFC107"
                        return "#F44336"
                    }
                }

                // Card 5: Flight State / Mode
                VGCSStatusCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    categoryLabel: "FLIGHT STATE"
                    primaryValue: isVehicleConnected ? (activeVehicle.flightMode ? activeVehicle.flightMode.toUpperCase() : "STANDBY") : (isCommLost ? "LINK LOST" : "STANDBY")
                    secondaryValue: isVehicleConnected ? (activeVehicle.armed ? "ARMED • LIVE" : "DISARMED • SAFE") : (isCommLost ? "UNREACHABLE" : "DISARMED")
                    statusColor: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
                }
            }
        }
    }
}
