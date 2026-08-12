/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Telemetry HUD Overlay
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: hudRoot

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null

    implicitHeight: 60
    implicitWidth: hudLayout.implicitWidth + 32
    radius: 4
    color: "#D9151C24" // 85% opacity dark surface
    border.color: "#2C3847"
    border.width: 1

    function getCardinal(headingDeg) {
        if (headingDeg === undefined || headingDeg === null || isNaN(headingDeg)) return "HDG"
        var val = Math.floor((headingDeg / 22.5) + 0.5)
        var arr = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"]
        return arr[(val % 16)]
    }

    RowLayout {
        id: hudLayout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 12

        // 1. Altitude (ALT)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 76
            Layout.preferredWidth: Math.max(altLabel.implicitWidth, altVal.implicitWidth, altSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: altLabel
                Layout.alignment: Qt.AlignHCenter
                text: "ALTITUDE"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: altVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.altitudeRelative && activeVehicle.altitudeRelative.value !== undefined && !isNaN(activeVehicle.altitudeRelative.value)) ? (activeVehicle.altitudeRelative.value.toFixed(1) + " m") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: (activeVehicle && activeVehicle.altitudeRelative && activeVehicle.altitudeRelative.value !== undefined && !isNaN(activeVehicle.altitudeRelative.value)) ? "#F5F7FA" : "#64748B"
            }
            Text {
                id: altSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.altitudeAMSL && activeVehicle.altitudeAMSL.value !== undefined && !isNaN(activeVehicle.altitudeAMSL.value)) ? ("AMSL " + activeVehicle.altitudeAMSL.value.toFixed(0) + "m") : "--"
                font.family: "JetBrains Mono"
                font.pixelSize: 8
                color: "#64748B"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 30; Layout.alignment: Qt.AlignVCenter; color: "#2C3847" }

        // 2. Speed (SPD)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 84
            Layout.preferredWidth: Math.max(spdLabel.implicitWidth, spdVal.implicitWidth, spdSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: spdLabel
                Layout.alignment: Qt.AlignHCenter
                text: "GROUND SPEED"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: spdVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.groundSpeed && activeVehicle.groundSpeed.value !== undefined && !isNaN(activeVehicle.groundSpeed.value)) ? (activeVehicle.groundSpeed.value.toFixed(1) + " m/s") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: (activeVehicle && activeVehicle.groundSpeed && activeVehicle.groundSpeed.value !== undefined && !isNaN(activeVehicle.groundSpeed.value)) ? "#F5F7FA" : "#64748B"
            }
            Text {
                id: spdSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.airSpeed && activeVehicle.airSpeed.value !== undefined && !isNaN(activeVehicle.airSpeed.value)) ? ("AIR " + activeVehicle.airSpeed.value.toFixed(1) + "m/s") : "--"
                font.family: "JetBrains Mono"
                font.pixelSize: 8
                color: "#64748B"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 30; Layout.alignment: Qt.AlignVCenter; color: "#2C3847" }

        // 3. Heading (HDG)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 72
            Layout.preferredWidth: Math.max(hdgLabel.implicitWidth, hdgVal.implicitWidth, hdgSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: hdgLabel
                Layout.alignment: Qt.AlignHCenter
                text: "HEADING"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: hdgVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.heading && activeVehicle.heading.value !== undefined && !isNaN(activeVehicle.heading.value)) ? (Math.round(activeVehicle.heading.value).toString().padStart(3, '0') + "°") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: (activeVehicle && activeVehicle.heading && activeVehicle.heading.value !== undefined && !isNaN(activeVehicle.heading.value)) ? "#F5F7FA" : "#64748B"
            }
            Text {
                id: hdgSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.heading && activeVehicle.heading.value !== undefined && !isNaN(activeVehicle.heading.value)) ? hudRoot.getCardinal(activeVehicle.heading.value) : "--"
                font.family: "JetBrains Mono"
                font.pixelSize: 8
                font.weight: Font.Bold
                color: (activeVehicle && activeVehicle.heading && activeVehicle.heading.value !== undefined && !isNaN(activeVehicle.heading.value)) ? "#FF6A00" : "#64748B"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 30; Layout.alignment: Qt.AlignVCenter; color: "#2C3847" }

        // 4. Vertical Speed (V/S)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 76
            Layout.preferredWidth: Math.max(vsLabel.implicitWidth, vsVal.implicitWidth, vsSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: vsLabel
                Layout.alignment: Qt.AlignHCenter
                text: "V/S (CLIMB)"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: vsVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.climbRate && activeVehicle.climbRate.value !== undefined && !isNaN(activeVehicle.climbRate.value)) ? ((activeVehicle.climbRate.value >= 0 ? "+" : "") + activeVehicle.climbRate.value.toFixed(1) + " m/s") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: {
                    if (!activeVehicle || !activeVehicle.climbRate || activeVehicle.climbRate.value === undefined || isNaN(activeVehicle.climbRate.value)) return "#64748B"
                    var v = activeVehicle.climbRate.value
                    if (v > 0.3) return "#00C853"
                    if (v < -0.3) return "#FFC107"
                    return "#F5F7FA"
                }
            }
            Text {
                id: vsSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.climbRate && activeVehicle.climbRate.value !== undefined && !isNaN(activeVehicle.climbRate.value)) ? "RATE" : "--"
                font.family: "Inter"
                font.pixelSize: 8
                color: "#64748B"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 30; Layout.alignment: Qt.AlignVCenter; color: "#2C3847" }

        // 5. GNSS (GPS)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 76
            Layout.preferredWidth: Math.max(gpsLabel.implicitWidth, gpsVal.implicitWidth, gpsSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: gpsLabel
                Layout.alignment: Qt.AlignHCenter
                text: "GNSS FIX"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: gpsVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " SATS") : (activeVehicle ? "NO FIX" : "N/A")
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: (activeVehicle && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? "#00C853" : (activeVehicle ? "#FFC107" : "#64748B")
            }
            Text {
                id: gpsSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.gps && activeVehicle.gps.hdop && activeVehicle.gps.hdop.value !== undefined && !isNaN(activeVehicle.gps.hdop.value)) ? ("HDOP " + activeVehicle.gps.hdop.value.toFixed(1)) : (activeVehicle ? "NO FIX" : "N/A")
                font.family: "JetBrains Mono"
                font.pixelSize: 8
                color: "#64748B"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 30; Layout.alignment: Qt.AlignVCenter; color: "#2C3847" }

        // 6. Battery (BAT)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 76
            Layout.preferredWidth: Math.max(batLabel.implicitWidth, batVal.implicitWidth, batSub.implicitWidth) + 12
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                id: batLabel
                Layout.alignment: Qt.AlignHCenter
                text: "BATTERY"
                font.family: "Inter"
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 0.6
                color: "#9BA8B5"
            }
            Text {
                id: batVal
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: {
                    if (!activeVehicle || !activeVehicle.battery || !activeVehicle.battery.percentRemaining || activeVehicle.battery.percentRemaining.value === undefined || isNaN(activeVehicle.battery.percentRemaining.value) || activeVehicle.battery.percentRemaining.value < 0) return "#64748B"
                    var pct = activeVehicle.battery.percentRemaining.value
                    if (pct > 30) return "#00C853"
                    if (pct > 15) return "#FFC107"
                    return "#F44336"
                }
            }
            Text {
                id: batSub
                Layout.alignment: Qt.AlignHCenter
                text: (activeVehicle && activeVehicle.battery && activeVehicle.battery.voltage && activeVehicle.battery.voltage.value !== undefined && !isNaN(activeVehicle.battery.voltage.value) && activeVehicle.battery.voltage.value > 0) ? (activeVehicle.battery.voltage.value.toFixed(1) + " V") : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 8
                color: "#64748B"
            }
        }
    }
}
