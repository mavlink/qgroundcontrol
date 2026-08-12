/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Vehicle Status Panel
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: statusPanelRoot

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    implicitWidth: 260
    implicitHeight: mainCol.implicitHeight + 24
    radius: 4
    color: "#E6151C24" // 90% opacity dark surface
    border.color: "#2C3847"
    border.width: 1

    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 10

        // Panel Title
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
            }

            Text {
                text: "VEHICLE STATUS"
                font.family: "Inter"
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 0.8
                color: "#F5F7FA"
                Layout.preferredWidth: implicitWidth
            }

            Item { Layout.fillWidth: true }

            Text {
                text: isVehicleConnected ? ("ID #" + activeVehicle.id) : (isCommLost ? ("ID #" + activeVehicle.id + " (LOST)") : "NO DRONE")
                font.family: "JetBrains Mono"
                font.pixelSize: 9
                font.weight: Font.Bold
                color: isVehicleConnected ? "#9BA8B5" : (isCommLost ? "#F44336" : "#64748B")
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // Section 1: Vehicle & Flight Mode
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "FLIGHT MODE"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: isVehicleConnected ? (activeVehicle.flightMode ? activeVehicle.flightMode.toUpperCase() : "STANDBY") : (isCommLost ? "LINK LOST" : "STANDBY")
                    font.family: "JetBrains Mono"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#F5F7FA") : (isCommLost ? "#F44336" : "#64748B")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "ARM STATE"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredHeight: 16
                    Layout.preferredWidth: armText.implicitWidth + 10
                    radius: 2
                    color: isVehicleConnected ? (activeVehicle.armed ? Qt.rgba(1.0, 0.416, 0.0, 0.2) : Qt.rgba(0.0, 0.784, 0.325, 0.2)) : "#1D2733"
                    border.color: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#00C853") : "#2C3847"
                    border.width: 1

                    Text {
                        id: armText
                        anchors.centerIn: parent
                        text: isVehicleConnected ? (activeVehicle.armed ? "ARMED" : "DISARMED") : (isCommLost ? "UNREACHABLE" : "DISARMED")
                        font.family: "JetBrains Mono"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        color: isVehicleConnected ? (activeVehicle.armed ? "#FF6A00" : "#00C853") : (isCommLost ? "#F44336" : "#64748B")
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "AIRFRAME"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: isVehicleConnected ? ((activeVehicle.vehicleTypeString && activeVehicle.vehicleTypeString.length > 0) ? activeVehicle.vehicleTypeString : (activeVehicle.px4Firmware ? "PX4 Pro Multirotor" : (activeVehicle.apmFirmware ? "ArduPilot UAS" : "Generic UAS"))) : "N/A"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Normal
                    color: isVehicleConnected ? "#F5F7FA" : "#64748B"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // Section 2: Communication Link
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "MAVLINK"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: isVehicleConnected ? "ONLINE v2.0" : (isCommLost ? "LINK LOST" : "OFFLINE")
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "PRIMARY LINK"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: isVehicleConnected ? (activeVehicle.vehicleLinkManager && activeVehicle.vehicleLinkManager.primaryLinkName ? activeVehicle.vehicleLinkManager.primaryLinkName : "CONNECTED") : (isCommLost ? "DISCONNECTED" : "NO LINK")
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: isVehicleConnected ? "#F5F7FA" : (isCommLost ? "#F44336" : "#64748B")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "SIGNAL RSSI"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: {
                        if (!isVehicleConnected) return "N/A"
                        if (typeof activeVehicle.telemetryLRSSI !== "undefined" && activeVehicle.telemetryLRSSI !== 0) {
                            return activeVehicle.telemetryLRSSI + " dBm"
                        }
                        if (typeof activeVehicle.rcRSSI !== "undefined" && activeVehicle.rcRSSI > 0 && activeVehicle.rcRSSI <= 100) {
                            return activeVehicle.rcRSSI + "%"
                        }
                        return "N/A"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    color: (isVehicleConnected && ((typeof activeVehicle.telemetryLRSSI !== "undefined" && activeVehicle.telemetryLRSSI !== 0) || (typeof activeVehicle.rcRSSI !== "undefined" && activeVehicle.rcRSSI > 0 && activeVehicle.rcRSSI <= 100))) ? "#F5F7FA" : "#64748B"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // Section 3: GNSS & Positioning
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "SATELLITES"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " SATS") : (activeVehicle ? "NO FIX" : "NO FIX")
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? "#00C853" : (activeVehicle ? "#FFC107" : "#64748B")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "HDOP / PRECISION"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: (activeVehicle && activeVehicle.gps && activeVehicle.gps.hdop && activeVehicle.gps.hdop.value !== undefined && !isNaN(activeVehicle.gps.hdop.value)) ? activeVehicle.gps.hdop.value.toFixed(2) : "N/A"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    color: (activeVehicle && activeVehicle.gps && activeVehicle.gps.hdop && activeVehicle.gps.hdop.value !== undefined && !isNaN(activeVehicle.gps.hdop.value)) ? "#F5F7FA" : "#64748B"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // Section 4: Power System
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "BATTERY REMAIN"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: (activeVehicle && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: {
                        if (!activeVehicle || !activeVehicle.battery || !activeVehicle.battery.percentRemaining || activeVehicle.battery.percentRemaining.value === undefined || isNaN(activeVehicle.battery.percentRemaining.value) || activeVehicle.battery.percentRemaining.value < 0) return "#64748B"
                        var pct = activeVehicle.battery.percentRemaining.value
                        if (pct > 30) return "#00C853"
                        if (pct > 15) return "#FFC107"
                        return "#F44336"
                    }
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "VOLTAGE / CURRENT"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: "#9BA8B5"
                    Layout.preferredWidth: implicitWidth
                }

                Text {
                    text: {
                        if (!activeVehicle || !activeVehicle.battery) return "N/A"
                        var hasV = activeVehicle.battery.voltage && activeVehicle.battery.voltage.value !== undefined && !isNaN(activeVehicle.battery.voltage.value) && activeVehicle.battery.voltage.value > 0
                        var hasA = activeVehicle.battery.current && activeVehicle.battery.current.value !== undefined && !isNaN(activeVehicle.battery.current.value)
                        if (!hasV && !hasA) return "N/A"
                        var vStr = hasV ? (activeVehicle.battery.voltage.value.toFixed(1) + "V") : ""
                        var aStr = hasA ? ((hasV ? " " : "") + activeVehicle.battery.current.value.toFixed(1) + "A") : ""
                        return vStr + aStr
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 9
                    color: (activeVehicle && activeVehicle.battery) ? "#F5F7FA" : "#64748B"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }
        }
    }
}
