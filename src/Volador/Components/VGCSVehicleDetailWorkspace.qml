/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Vehicle Detail Workspace
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: detailRoot

    property var vehicle: null
    readonly property bool isCommLost: vehicle !== null && vehicle.vehicleLinkManager !== null && vehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: vehicle !== null && vehicle.vehicleLinkManager !== null && !vehicle.vehicleLinkManager.communicationLost
    readonly property bool isActive: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager && vehicle && QGroundControl.multiVehicleManager.activeVehicle === vehicle)

    signal backToFleetRequested()

    color: "#0A0F14"

    function getCardinal(headingDeg) {
        if (headingDeg === undefined || headingDeg === null || isNaN(headingDeg)) return "HDG"
        var val = Math.floor((headingDeg / 22.5) + 0.5)
        var arr = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"]
        return arr[(val % 16)]
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---------------------------------------------------------------------
        // 1. VEHICLE DETAIL HEADER (48px)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#0A0F14"
            border.color: "#2C3847"
            border.width: 1
            z: 10

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                // Back to Fleet Button
                Rectangle {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: backRow.implicitWidth + 16
                    radius: 4
                    color: backMouse.containsMouse ? "#2C3847" : "#151C24"
                    border.color: backMouse.containsMouse ? "#FF6A00" : "#2C3847"
                    border.width: 1

                    RowLayout {
                        id: backRow
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "◀"
                            font.pixelSize: 10
                            color: "#FF6A00"
                        }

                        Text {
                            text: "FLEET OVERVIEW"
                            font.family: "Inter"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: "#F5F7FA"
                        }
                    }

                    MouseArea {
                        id: backMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailRoot.backToFleetRequested()
                    }
                }

                Rectangle { width: 1; height: 20; color: "#2C3847" }

                // Vehicle Name & Type
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    RowLayout {
                        spacing: 8

                        Text {
                            text: isVehicleConnected ? ("DRONE #" + vehicle.id) : (isCommLost ? ("DRONE #" + vehicle.id) : "NO VEHICLE")
                            font.family: "JetBrains Mono"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                            color: "#F5F7FA"
                        }

                        Rectangle {
                            width: 6
                            height: 6
                            radius: 3
                            color: isVehicleConnected ? (vehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
                        }
                    }

                    Text {
                        text: {
                            if (!vehicle) return "DISCONNECTED"
                            if (isCommLost) return "CONNECTION LOST"
                            if (vehicle.vehicleTypeString && vehicle.vehicleTypeString.length > 0) return vehicle.vehicleTypeString
                            if (vehicle.px4Firmware) return "PX4 Pro Multirotor"
                            if (vehicle.apmFirmware) return "ArduPilot UAS"
                            return "Generic UAS"
                        }
                        font.family: "Inter"
                        font.pixelSize: 9
                        color: isCommLost ? "#F44336" : "#9BA8B5"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                // Header Badges
                RowLayout {
                    spacing: 8

                    // Connection Badge
                    Rectangle {
                        height: 26
                        implicitWidth: connLabel.implicitWidth + 18
                        radius: 3
                        color: "#151C24"
                        border.color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#2C3847")
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 5

                            Rectangle {
                                width: 5
                                height: 5
                                radius: 2.5
                                color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                            }

                            Text {
                                id: connLabel
                                text: isVehicleConnected ? "ONLINE" : (isCommLost ? "LINK LOST" : "OFFLINE")
                                font.family: "Inter"
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                            }
                        }
                    }

                    // Active State Badge
                    Rectangle {
                        height: 26
                        implicitWidth: activeBadgeLabel.implicitWidth + 14
                        radius: 3
                        color: isActive ? Qt.rgba(1.0, 0.416, 0.0, 0.2) : "#151C24"
                        border.color: isActive ? "#FF6A00" : "#2C3847"
                        border.width: 1

                        Text {
                            id: activeBadgeLabel
                            anchors.centerIn: parent
                            text: isActive ? "ACTIVE VEHICLE" : "STANDBY"
                            font.family: "Inter"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: isActive ? "#FF6A00" : "#9BA8B5"
                        }
                    }

                    // Set Active Action
                    Rectangle {
                        height: 26
                        implicitWidth: setActiveLabel.implicitWidth + 16
                        radius: 3
                        color: isActive ? "#1D2733" : (setActiveMouse.containsMouse ? "#FF6A00" : "#151C24")
                        border.color: isActive ? "#FF6A00" : (setActiveMouse.containsMouse ? "#FF6A00" : "#2C3847")
                        border.width: 1
                        visible: !isActive && !!vehicle

                        Text {
                            id: setActiveLabel
                            anchors.centerIn: parent
                            text: "CONTROL THIS DRONE"
                            font.family: "Inter"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: setActiveMouse.containsMouse ? "#0A0F14" : "#F5F7FA"
                        }

                        MouseArea {
                            id: setActiveMouse
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
                }
            }
        }

        // ---------------------------------------------------------------------
        // 2. MAIN CONTENT AREA
        // ---------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // State A: Empty state (no vehicle)
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                visible: !vehicle

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 64
                    height: 64
                    radius: 32
                    color: "#151C24"
                    border.color: "#2C3847"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "!"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 24
                        font.weight: Font.Bold
                        color: "#9BA8B5"
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "NO VEHICLE SELECTED"
                    font.family: "Inter"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Select a vehicle from FLEET OVERVIEW to view detailed telemetry and subsystem information."
                    font.family: "Inter"
                    font.pixelSize: 10
                    color: "#9BA8B5"
                }
            }

            // State B: Vehicle Details View
            ScrollView {
                id: detailScrollView
                anchors.fill: parent
                anchors.margins: 20
                visible: !!vehicle
                clip: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: detailScrollView.availableWidth
                    spacing: 16

                    // =========================================================
                    // Section 1: Flight State & Primary Telemetry Card
                    // =========================================================
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: flightStateCol.implicitHeight + 32
                        radius: 4
                        color: "#151C24"
                        border.color: "#2C3847"
                        border.width: 1

                        ColumnLayout {
                            id: flightStateCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 16
                            spacing: 12

                            // Card Title Row
                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: "FLIGHT STATE & TELEMETRY"
                                    font.family: "Inter"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: "#F5F7FA"
                                    font.letterSpacing: 0.5
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    implicitHeight: 22
                                    implicitWidth: armedLabel.implicitWidth + 12
                                    radius: 3
                                    color: vehicle && vehicle.armed ? Qt.rgba(1.0, 0.416, 0.0, 0.15) : Qt.rgba(0.0, 0.784, 0.325, 0.15)
                                    border.color: vehicle && vehicle.armed ? "#FF6A00" : "#00C853"
                                    border.width: 1

                                    Text {
                                        id: armedLabel
                                        anchors.centerIn: parent
                                        text: vehicle ? (vehicle.armed ? "ARMED • LIVE MOTORS" : "DISARMED • SAFE") : "STANDBY"
                                        font.family: "Inter"
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        color: vehicle ? (vehicle.armed ? "#FF6A00" : "#00C853") : "#64748B"
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

                            // 4 Telemetry Columns Grid
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                // Tile 1: Flight Mode
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.minimumWidth: 120
                                    implicitHeight: tile1Col.implicitHeight + 16
                                    radius: 3
                                    color: "#0F161E"
                                    border.color: "#1F2B38"
                                    border.width: 1

                                    ColumnLayout {
                                        id: tile1Col
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 10
                                        spacing: 3

                                        Text {
                                            text: "FLIGHT MODE"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: vehicle ? (vehicle.flightMode ? vehicle.flightMode.toUpperCase() : "STANDBY") : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            font.weight: Font.Bold
                                            color: "#F5F7FA"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: vehicle ? (vehicle.armed ? "ACTIVE CONTROL" : "MOTORS SAFE") : "STANDBY"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 8
                                            color: vehicle && vehicle.armed ? "#FF6A00" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                // Tile 2: Relative Altitude
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.minimumWidth: 120
                                    implicitHeight: tile2Col.implicitHeight + 16
                                    radius: 3
                                    color: "#0F161E"
                                    border.color: "#1F2B38"
                                    border.width: 1

                                    ColumnLayout {
                                        id: tile2Col
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 10
                                        spacing: 3

                                        Text {
                                            text: "ALTITUDE (REL)"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.altitudeRelative && vehicle.altitudeRelative.value !== undefined && !isNaN(vehicle.altitudeRelative.value)) ? (vehicle.altitudeRelative.value.toFixed(1) + " m") : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            font.weight: Font.Bold
                                            color: (vehicle && vehicle.altitudeRelative && vehicle.altitudeRelative.value !== undefined && !isNaN(vehicle.altitudeRelative.value)) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.altitudeAMSL && vehicle.altitudeAMSL.value !== undefined && !isNaN(vehicle.altitudeAMSL.value)) ? ("AMSL " + vehicle.altitudeAMSL.value.toFixed(1) + " m") : "AMSL --"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 8
                                            color: "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                // Tile 3: Ground Speed
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.minimumWidth: 120
                                    implicitHeight: tile3Col.implicitHeight + 16
                                    radius: 3
                                    color: "#0F161E"
                                    border.color: "#1F2B38"
                                    border.width: 1

                                    ColumnLayout {
                                        id: tile3Col
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 10
                                        spacing: 3

                                        Text {
                                            text: "GROUND SPEED"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.groundSpeed && vehicle.groundSpeed.value !== undefined && !isNaN(vehicle.groundSpeed.value)) ? (vehicle.groundSpeed.value.toFixed(1) + " m/s") : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            font.weight: Font.Bold
                                            color: (vehicle && vehicle.groundSpeed && vehicle.groundSpeed.value !== undefined && !isNaN(vehicle.groundSpeed.value)) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.airSpeed && vehicle.airSpeed.value !== undefined && !isNaN(vehicle.airSpeed.value)) ? ("AIR " + vehicle.airSpeed.value.toFixed(1) + " m/s") : "AIR --"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 8
                                            color: "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                // Tile 4: Heading
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    Layout.minimumWidth: 120
                                    implicitHeight: tile4Col.implicitHeight + 16
                                    radius: 3
                                    color: "#0F161E"
                                    border.color: "#1F2B38"
                                    border.width: 1

                                    ColumnLayout {
                                        id: tile4Col
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 10
                                        spacing: 3

                                        Text {
                                            text: "HEADING"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.heading && vehicle.heading.value !== undefined && !isNaN(vehicle.heading.value)) ? (Math.round(vehicle.heading.value).toString().padStart(3, '0') + "°") : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 13
                                            font.weight: Font.Bold
                                            color: (vehicle && vehicle.heading && vehicle.heading.value !== undefined && !isNaN(vehicle.heading.value)) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.heading && vehicle.heading.value !== undefined && !isNaN(vehicle.heading.value)) ? detailRoot.getCardinal(vehicle.heading.value) : "HDG --"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 8
                                            font.weight: Font.Bold
                                            color: (vehicle && vehicle.heading && vehicle.heading.value !== undefined && !isNaN(vehicle.heading.value)) ? "#FF6A00" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // =========================================================
                    // Section 2: Subsystems & System Health Grid (2 Cards)
                    // =========================================================
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        // -----------------------------------------------------
                        // Subsystem Card A: Communication & GNSS (50% width)
                        // -----------------------------------------------------
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.minimumWidth: 320
                            implicitHeight: subACol.implicitHeight + 32
                            radius: 4
                            color: "#151C24"
                            border.color: "#2C3847"
                            border.width: 1

                            ColumnLayout {
                                id: subACol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 16
                                spacing: 12

                                Text {
                                    text: "COMMUNICATION & GNSS"
                                    font.family: "Inter"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: "#F5F7FA"
                                    font.letterSpacing: 0.5
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 14
                                    columnSpacing: 20

                                    // Metric 1: MAVLink Protocol
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "MAVLINK PROTOCOL"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: isVehicleConnected ? "ONLINE v2.0" : (isCommLost ? "LINK LOST" : "OFFLINE")
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 2: GNSS Satellites
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "GNSS SATELLITES"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (isVehicleConnected && vehicle.gps && vehicle.gps.count && vehicle.gps.count.value !== undefined && !isNaN(vehicle.gps.count.value) && vehicle.gps.count.value > 0) ? (vehicle.gps.count.value + " SATS") : (isCommLost ? "NO FIX (LOST)" : "NO FIX")
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: (isVehicleConnected && vehicle.gps && vehicle.gps.count && vehicle.gps.count.value !== undefined && vehicle.gps.count.value >= 6) ? "#00C853" : (isCommLost ? "#F44336" : (isVehicleConnected ? "#FFC107" : "#64748B"))
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 3: Primary Physical Link
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "PRIMARY LINK"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: isVehicleConnected ? (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.primaryLinkName ? vehicle.vehicleLinkManager.primaryLinkName : "LINK ATTACHED") : (isCommLost ? "DISCONNECTED" : "NO LINK")
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: isVehicleConnected ? "#F5F7FA" : (isCommLost ? "#F44336" : "#64748B")
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 4: HDOP Precision
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "HDOP / PRECISION"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (isVehicleConnected && vehicle.gps && vehicle.gps.hdop && vehicle.gps.hdop.value !== undefined && !isNaN(vehicle.gps.hdop.value)) ? vehicle.gps.hdop.value.toFixed(2) : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            color: (isVehicleConnected && vehicle.gps && vehicle.gps.hdop && vehicle.gps.hdop.value !== undefined && !isNaN(vehicle.gps.hdop.value)) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 5: Signal RSSI
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "SIGNAL RSSI"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
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
                                                return "N/A"
                                            }
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            color: (isVehicleConnected && ((typeof vehicle.telemetryLRSSI !== "undefined" && vehicle.telemetryLRSSI !== 0) || (typeof vehicle.rcRSSI !== "undefined" && vehicle.rcRSSI > 0 && vehicle.rcRSSI <= 100))) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 6: GNSS Fix Status
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "POSITION STATUS"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.gps && vehicle.gps.lock && vehicle.gps.lock.enumStringValue) ? vehicle.gps.lock.enumStringValue.toUpperCase() : (vehicle ? "NO FIX" : "N/A")
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: (vehicle && vehicle.gps && vehicle.gps.count && vehicle.gps.count.value >= 6) ? "#00C853" : (vehicle ? "#FFC107" : "#64748B")
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }

                        // -----------------------------------------------------
                        // Subsystem Card B: Power System & Health (50% width)
                        // -----------------------------------------------------
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.minimumWidth: 320
                            implicitHeight: subBCol.implicitHeight + 32
                            radius: 4
                            color: "#151C24"
                            border.color: "#2C3847"
                            border.width: 1

                            ColumnLayout {
                                id: subBCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 16
                                spacing: 12

                                Text {
                                    text: "POWER SYSTEM & HARDWARE"
                                    font.family: "Inter"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: "#F5F7FA"
                                    font.letterSpacing: 0.5
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 14
                                    columnSpacing: 20

                                    // Metric 1: Battery Percent
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "BATTERY REMAINING"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: (vehicle && vehicle.battery && vehicle.battery.percentRemaining && vehicle.battery.percentRemaining.value !== undefined && !isNaN(vehicle.battery.percentRemaining.value) && vehicle.battery.percentRemaining.value >= 0) ? (Math.round(vehicle.battery.percentRemaining.value) + "%") : "N/A"
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
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
                                    }

                                    // Metric 2: System Status
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "SYSTEM STATUS"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: isVehicleConnected ? "OPERATIONAL" : (isCommLost ? "LINK LOST" : "OFFLINE")
                                            font.family: "Inter"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 3: Voltage & Current
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "VOLTAGE / CURRENT"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: {
                                                if (!vehicle || !vehicle.battery) return "N/A"
                                                var hasV = vehicle.battery.voltage && vehicle.battery.voltage.value !== undefined && !isNaN(vehicle.battery.voltage.value) && vehicle.battery.voltage.value > 0
                                                var hasA = vehicle.battery.current && vehicle.battery.current.value !== undefined && !isNaN(vehicle.battery.current.value)
                                                if (!hasV && !hasA) return "N/A"
                                                var vStr = hasV ? (vehicle.battery.voltage.value.toFixed(1) + " V") : ""
                                                var aStr = hasA ? ((hasV ? " • " : "") + vehicle.battery.current.value.toFixed(1) + " A") : ""
                                                return vStr + aStr
                                            }
                                            font.family: "JetBrains Mono"
                                            font.pixelSize: 11
                                            color: (vehicle && vehicle.battery) ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Metric 4: Hardware / Autopilot
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        spacing: 3

                                        Text {
                                            text: "AUTOPILOT / FIRMWARE"
                                            font.family: "Inter"
                                            font.pixelSize: 8
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                            color: "#9BA8B5"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: vehicle ? (vehicle.px4Firmware ? "PX4 Autopilot" : (vehicle.apmFirmware ? "ArduPilot Autopilot" : "Generic MAVLink")) : "N/A"
                                            font.family: "Inter"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            color: vehicle ? "#F5F7FA" : "#64748B"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
