/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap

Rectangle {
    id: root
    property var vehicle

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    color: qgcPal.windowShade
    border.color: vehicle === QGroundControl.multiVehicleManager.activeVehicle
                  ? qgcPal.colorBlue
                  : qgcPal.text
    border.width: vehicle === QGroundControl.multiVehicleManager.activeVehicle ? 3 : 1
    radius: ScreenTools.defaultFontPixelWidth * 0.5

    // ===============================
    // Battery (multi-battery safe)
    // ===============================
    function worstBattery() {
        if (!vehicle || !vehicle.batteries || vehicle.batteries.count === 0)
            return null

        var worst = null
        var lowest = 101
        for (var i = 0; i < vehicle.batteries.count; i++) {
            var b = vehicle.batteries.get(i)
            if (!b || isNaN(b.percentRemaining.rawValue)) continue
            if (b.percentRemaining.rawValue < lowest) {
                lowest = b.percentRemaining.rawValue
                worst = b
            }
        }
        return worst
    }

    function batteryPercent() {
        var b = worstBattery()
        return b ? b.percentRemaining.valueString + b.percentRemaining.units : "N/A"
    }

    function batteryColor() {
        var b = worstBattery()
        if (!b) return qgcPal.text
        if (b.percentRemaining.rawValue > 50) return qgcPal.colorGreen
        if (b.percentRemaining.rawValue > 20) return qgcPal.colorOrange
        return qgcPal.colorRed
    }

    // ===============================
    // Layout
    // ===============================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.6
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        // Vehicle name
        QGCLabel {
            text: (vehicle.vehicleTypeName || "Vehicle") + " " + vehicle.id
            font.bold: true
            font.pointSize: ScreenTools.mediumFontPointSize
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Mode
        QGCLabel {
            text: "MODE: " + (vehicle.flightMode || "N/A")
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorBlue
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Battery row
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            Rectangle {
                width: 14
                height: 14
                radius: 7
                color: batteryColor()
            }

            QGCLabel {
                text: batteryPercent()
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }

        // ===============================
        // Compass + Attitude
        // ===============================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 6
            color: qgcPal.windowShade
            radius: ScreenTools.defaultFontPixelWidth * 0.4

            IntegratedCompassAttitude {
                anchors.centerIn: parent
                compassRadius:              parent.height * 0.42
                compassBorder:              0
                attitudeSize:               ScreenTools.defaultFontPixelWidth / 2
                attitudeSpacing:            attitudeSize / 2
                usedByMultipleVehicleList:   true
                vehicle:                    root.vehicle
            }
        }

        // ===============================
        // Altitude
        // ===============================
             QGCLabel {
                    font.pointSize: ScreenTools.mediumFontPointSize
                    color: qgcPal.colorBlue
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        if (!vehicle)
                            return "N/A"

                        // Prefer AGL if available
                        if (vehicle.altitudeRelative && !isNaN(vehicle.altitudeRelative.rawValue))
                            return vehicle.altitudeRelative.valueString + " " +
                                   vehicle.altitudeRelative.units

                        // Fallback to AMSL
                        if (vehicle.altitudeAMSL && !isNaN(vehicle.altitudeAMSL.rawValue))
                            return vehicle.altitudeAMSL.valueString + " " +
                                   vehicle.altitudeAMSL.units

                        return "N/A"
                    }
                }


        // Armed / Landed
        QGCLabel {
            text: vehicle.armed ? "ARMED" : "LANDED"
            font.bold: true
            font.pointSize: ScreenTools.smallFontPointSize
            color: vehicle.armed ? qgcPal.colorOrange : qgcPal.colorGreen
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: QGroundControl.multiVehicleManager.activeVehicle = vehicle
    }
}
