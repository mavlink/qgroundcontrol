/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Header Bar Component
 *
 * Top Landscape Telemetry & Security Header Bar
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id: root
    implicitHeight: ScreenTools.defaultFontPixelHeight * 2.8
    color: "#141923"

    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#263044"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 2
        spacing: ScreenTools.defaultFontPixelWidth * 2

        // Brand & Title
        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 1.5

            Image {
                source: "qrc:/Volador/Assets/Logos/volador_primary.png"
                sourceSize.height: ScreenTools.defaultFontPixelHeight * 1.8
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                mipmap: true
            }

            ColumnLayout {
                spacing: 0
                QGCLabel {
                    text: "VGCS"
                    font.pointSize: ScreenTools.mediumFontPointSize
                    font.bold: true
                    color: "#FFFFFF"
                }
                QGCLabel {
                    text: qsTr("Enterprise Flight Control")
                    font.pointSize: ScreenTools.smallFontPointSize - 2
                    color: "#D1D5DB"
                }
            }
        }

        Item { Layout.fillWidth: true } // Spacer

        // Telemetry Summary Badges
        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 3

            // Vehicle Connection Status
            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: isVehicleConnected ? "#2E7D32" : (isCommLost ? "#E53935" : "#6B7280")
                }
                QGCLabel {
                    text: isVehicleConnected ? qsTr("Vehicle Online") : (isCommLost ? qsTr("Link Lost") : qsTr("No Vehicle"))
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: qgcPal.text
                }
            }

            // GPS Status
            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                Image {
                    source: "qrc:/qmlimages/Gps.svg"
                    sourceSize.height: ScreenTools.defaultFontPixelHeight * 1.2
                    fillMode: Image.PreserveAspectFit
                }
                QGCLabel {
                    text: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && !isNaN(activeVehicle.gps.count.value) && activeVehicle.gps.count.value > 0) ? (activeVehicle.gps.count.value + " Sats") : (isCommLost ? qsTr("No Fix (Lost)") : (isVehicleConnected ? qsTr("No Fix") : qsTr("N/A")))
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: (isVehicleConnected && activeVehicle.gps && activeVehicle.gps.count && activeVehicle.gps.count.value !== undefined && activeVehicle.gps.count.value >= 6) ? "#2E7D32" : (isCommLost ? "#E53935" : "#D1D5DB")
                }
            }

            // Battery Status
            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                Image {
                    source: "qrc:/qmlimages/Battery.svg"
                    sourceSize.height: ScreenTools.defaultFontPixelHeight * 1.2
                    fillMode: Image.PreserveAspectFit
                }
                QGCLabel {
                    text: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? (Math.round(activeVehicle.battery.percentRemaining.value) + "%") : "N/A"
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: (isVehicleConnected && activeVehicle.battery && activeVehicle.battery.percentRemaining && activeVehicle.battery.percentRemaining.value !== undefined && !isNaN(activeVehicle.battery.percentRemaining.value) && activeVehicle.battery.percentRemaining.value >= 0) ? "#2E7D32" : (isCommLost ? "#E53935" : "#9E9E9E")
                }
            }
        }

        Item { Layout.fillWidth: true } // Spacer

        // Active Security Role & User Badge
        Rectangle {
            implicitWidth: userRow.width + (ScreenTools.defaultFontPixelWidth * 2)
            implicitHeight: ScreenTools.defaultFontPixelHeight * 2
            color: "#2B2F33"
            radius: 6
            border.color: "#40464D"

            RowLayout {
                id: userRow
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: {
                        var role = voladorAuth ? voladorAuth.userRole : ""
                        if (role === "Administrator") return "#E53935"
                        if (role === "MissionPlanner") return "#6B7280"
                        if (role === "Pilot") return "#2E7D32"
                        return "#6B7280"
                    }
                }

                QGCLabel {
                    text: voladorAuth ? (voladorAuth.currentUser + " [" + voladorAuth.userRole + "]") : "Guest"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
        }

        // Logout Button
        QGCButton {
            text: qsTr("Logout")
            onClicked: {
                if (voladorAuth) {
                    voladorAuth.logout()
                }
            }
        }
    }
}
