/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Vehicle Management Workspace
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

    property string viewMode: "fleet" // "fleet" or "detail"
    property var selectedVehicle: null

    readonly property var vehicleManager: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager : null
    readonly property var vehiclesList: (vehicleManager && vehicleManager.vehicles) ? vehicleManager.vehicles : null
    readonly property int vehicleCount: (vehiclesList && typeof vehiclesList.count !== "undefined") ? vehiclesList.count : 0
    readonly property var activeVehicle: vehicleManager ? vehicleManager.activeVehicle : null

    color: "#0A0F14"

    // Fleet Grid View
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: workspaceRoot.viewMode === "fleet"

        // ---------------------------------------------------------------------
        // 1. WORKSPACE HEADER (48px)
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
                spacing: 16

                // Title & Subtitle
                Column {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 1

                    Text {
                        text: "VEHICLE MANAGEMENT"
                        font.family: "Inter"
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: "#F5F7FA"
                    }

                    Text {
                        text: "VEHICLE FLEET, CONNECTION AND SYSTEM STATUS"
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Normal
                        color: "#9BA8B5"
                    }
                }

                Item { Layout.fillWidth: true }

                // Right Badges
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8

                    // Connected Vehicles Count Chip
                    Rectangle {
                        height: 28
                        implicitWidth: countRow.implicitWidth + 16
                        radius: 4
                        color: "#151C24"
                        border.color: vehicleCount > 0 ? "#00C853" : "#2C3847"
                        border.width: 1

                        RowLayout {
                            id: countRow
                            anchors.centerIn: parent
                            spacing: 6

                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: vehicleCount > 0 ? "#00C853" : "#64748B"
                            }

                            Text {
                                text: vehicleCount + (vehicleCount === 1 ? " VEHICLE CONNECTED" : " VEHICLES CONNECTED")
                                font.family: "JetBrains Mono"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: vehicleCount > 0 ? "#F5F7FA" : "#9BA8B5"
                            }
                        }
                    }

                    // Active Vehicle Chip
                    Rectangle {
                        height: 28
                        implicitWidth: activeText.implicitWidth + 16
                        radius: 4
                        color: activeVehicle ? Qt.rgba(1.0, 0.416, 0.0, 0.15) : "#151C24"
                        border.color: activeVehicle ? "#FF6A00" : "#2C3847"
                        border.width: 1

                        Text {
                            id: activeText
                            anchors.centerIn: parent
                            text: activeVehicle ? ("ACTIVE: DRONE #" + activeVehicle.id) : "NO ACTIVE DRONE"
                            font.family: "JetBrains Mono"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: activeVehicle ? "#FF6A00" : "#64748B"
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // 2. MAIN CONTENT AREA: FLEET GRID OR CLEAN EMPTY STATE
        // ---------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // State A: Fleet Grid (When vehicles > 0)
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                visible: vehicleCount > 0
                clip: true

                Flow {
                    width: parent.width
                    spacing: 16

                    Repeater {
                        model: vehiclesList
                        delegate: VGCSVehicleCard {
                            vehicle: object
                            onInspectRequested: function(veh) {
                                workspaceRoot.selectedVehicle = veh
                                workspaceRoot.viewMode = "detail"
                            }
                        }
                    }
                }
            }

            // State B: Clean Empty State (When vehicles == 0)
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 14
                visible: vehicleCount === 0

                // Empty State Graphic
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 72
                    height: 72
                    radius: 36
                    color: "#151C24"
                    border.color: "#2C3847"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "✈"
                        font.pixelSize: 28
                        color: "#64748B"
                    }
                }

                // Primary Message
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "NO VEHICLES CONNECTED"
                    font.family: "Inter"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }

                // Secondary Description
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Connect a vehicle to begin vehicle monitoring and configuration."
                    font.family: "Inter"
                    font.pixelSize: 11
                    color: "#9BA8B5"
                }

                // Status Indicator
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    height: 24
                    implicitWidth: waitingText.implicitWidth + 16
                    radius: 3
                    color: "#151C24"
                    border.color: "#FF6A00"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6

                        Rectangle {
                            width: 6
                            height: 6
                            radius: 3
                            color: "#FF6A00"
                        }

                        Text {
                            id: waitingText
                            text: "LISTENING FOR MAVLINK COMMS..."
                            font.family: "JetBrains Mono"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: "#FF6A00"
                        }
                    }
                }
            }
        }
    }

    // Vehicle Detail Workspace Container
    VGCSVehicleDetailWorkspace {
        anchors.fill: parent
        visible: workspaceRoot.viewMode === "detail"
        vehicle: workspaceRoot.selectedVehicle ? workspaceRoot.selectedVehicle : workspaceRoot.activeVehicle
        onBackToFleetRequested: {
            workspaceRoot.viewMode = "fleet"
        }
    }
}
