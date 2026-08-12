/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Mission Planning Workspace
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
    id: missionWorkspaceRoot

    property var planMasterController: null
    property var missionController: planMasterController ? planMasterController.missionController : null
    property var planView: null
    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null
    readonly property bool isCommLost: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost

    readonly property real headerHeight: missionHeader.height
    readonly property real bottomBarHeight: missionStatusBar.height

    color: "transparent"

    // -------------------------------------------------------------------------
    // 1. TOP MISSION HEADER (48px)
    // -------------------------------------------------------------------------
    Rectangle {
        id: missionHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: "#0A0F14"
        border.color: "#2C3847"
        border.width: 1
        z: 20

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Left: Title & Subtitle
            Column {
                Layout.alignment: Qt.AlignVCenter
                spacing: 1

                Text {
                    text: "MISSION PLANNING"
                    font.family: "Inter"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    font.letterSpacing: 0.8
                    color: "#F5F7FA"
                }

                Text {
                    text: "AEROSPACE ROUTE DESIGN & FLIGHT PLAN MANAGEMENT"
                    font.family: "Inter"
                    font.pixelSize: 10
                    font.weight: Font.Normal
                    color: "#9BA8B5"
                }
            }

            Item { Layout.fillWidth: true }

            // Right: Status Badges & Vehicle Chip
            RowLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                // Vehicle Connection Chip
                Rectangle {
                    height: 28
                    implicitWidth: connRow.implicitWidth + 16
                    radius: 4
                    color: "#151C24"
                    border.color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#2C3847")
                    border.width: 1

                    RowLayout {
                        id: connRow
                        anchors.centerIn: parent
                        spacing: 6

                        Rectangle {
                            width: 6
                            height: 6
                            radius: 3
                            color: isVehicleConnected ? "#00C853" : (isCommLost ? "#F44336" : "#64748B")
                        }

                        Text {
                            text: isVehicleConnected ? ("DRONE #" + activeVehicle.id) : (isCommLost ? ("DRONE #" + activeVehicle.id + " • LOST") : "OFFLINE PLANNER")
                            font.family: "Inter"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: isVehicleConnected ? "#F5F7FA" : (isCommLost ? "#F44336" : "#9BA8B5")
                        }
                    }
                }

                // Mission Dirty / Synced State Badge
                Rectangle {
                    height: 28
                    implicitWidth: dirtyText.implicitWidth + 16
                    radius: 4
                    color: (planMasterController && planMasterController.dirty) ? Qt.rgba(1.0, 0.416, 0.0, 0.15) : "#151C24"
                    border.color: (planMasterController && planMasterController.dirty) ? "#FF6A00" : "#2C3847"
                    border.width: 1
                    visible: !!planMasterController

                    Text {
                        id: dirtyText
                        anchors.centerIn: parent
                        text: (planMasterController && planMasterController.dirty) ? "UNSAVED CHANGES" : "PLAN SYNCED"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: (planMasterController && planMasterController.dirty) ? "#FF6A00" : "#00C853"
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 2. MISSION TOOLBAR (Floating Top-Center)
    // -------------------------------------------------------------------------
    VGCSMissionToolbar {
        id: missionToolbar
        anchors.top: missionHeader.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        z: 30
        planMasterController: missionWorkspaceRoot.planMasterController
        planView: missionWorkspaceRoot.planView
    }

    // -------------------------------------------------------------------------
    // 3. WAYPOINT SEQUENCE LIST (Floating Left Drawer)
    // -------------------------------------------------------------------------
    VGCSMissionWaypointList {
        id: waypointList
        anchors.top: missionHeader.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.bottom: missionStatusBar.top
        anchors.bottomMargin: 10
        z: 30
        planMasterController: missionWorkspaceRoot.planMasterController
    }

    // -------------------------------------------------------------------------
    // 4. MISSION ITEM INSPECTOR (Floating Top-Right)
    // -------------------------------------------------------------------------
    VGCSMissionItemInspector {
        id: missionInspector
        anchors.top: missionHeader.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 12
        z: 30
        planMasterController: missionWorkspaceRoot.planMasterController
    }

    // -------------------------------------------------------------------------
    // 5. BOTTOM MISSION STATUS HUD BAR (38px)
    // -------------------------------------------------------------------------
    Rectangle {
        id: missionStatusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 38
        color: "#0F161E"
        border.color: "#2C3847"
        border.width: 1
        z: 20

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 16

            // Metric 1: Planned Distance
            RowLayout {
                spacing: 6
                Text {
                    text: "DISTANCE"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    color: "#9BA8B5"
                }
                Text {
                    text: {
                        if (!missionController || missionController.missionPlannedDistance <= 0) return "--"
                        var d = missionController.missionPlannedDistance
                        return d >= 1000 ? (d / 1000.0).toFixed(2) + " km" : d.toFixed(0) + " m"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }
            }

            Rectangle { width: 1; height: 16; color: "#2C3847" }

            // Metric 2: Waypoints Count
            RowLayout {
                spacing: 6
                Text {
                    text: "WAYPOINTS"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    color: "#9BA8B5"
                }
                Text {
                    text: {
                        if (!missionController || !missionController.visualItems) return "0"
                        var c = missionController.visualItems.count
                        return c > 1 ? (c - 1).toString() : "0"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }
            }

            Rectangle { width: 1; height: 16; color: "#2C3847" }

            // Metric 3: Max Altitude
            RowLayout {
                spacing: 6
                Text {
                    text: "MAX ALTITUDE"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    color: "#9BA8B5"
                }
                Text {
                    text: {
                        if (missionController && missionController.maxAMSLAltitude > 0) {
                            return missionController.maxAMSLAltitude.toFixed(0) + " m"
                        }
                        if (typeof QGroundControl !== "undefined" && QGroundControl.settingsManager && QGroundControl.settingsManager.appSettings) {
                            return QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude.rawValue.toFixed(0) + " m"
                        }
                        return "--"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }
            }

            Rectangle { width: 1; height: 16; color: "#2C3847" }

            // Metric 4: Estimated Time
            RowLayout {
                spacing: 6
                Text {
                    text: "EST. TIME"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    color: "#9BA8B5"
                }
                Text {
                    text: {
                        if (!missionController || missionController.missionTime <= 0) return "--:--"
                        var t = missionController.missionTime
                        var mins = Math.floor(t / 60)
                        var secs = Math.floor(t % 60)
                        return mins + "m " + (secs < 10 ? "0" : "") + secs + "s"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }
            }

            Item { Layout.fillWidth: true }

            // Metric 5: Mission State Badge
            RowLayout {
                spacing: 6
                Text {
                    text: "MISSION STATE"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    color: "#9BA8B5"
                }

                Rectangle {
                    height: 22
                    implicitWidth: stateLabel.implicitWidth + 12
                    radius: 3
                    color: {
                        if (planMasterController && planMasterController.syncInProgress) return Qt.rgba(1.0, 0.757, 0.027, 0.2)
                        if (planMasterController && planMasterController.dirty) return Qt.rgba(1.0, 0.416, 0.0, 0.2)
                        return Qt.rgba(0.0, 0.784, 0.325, 0.2)
                    }
                    border.color: {
                        if (planMasterController && planMasterController.syncInProgress) return "#FFC107"
                        if (planMasterController && planMasterController.dirty) return "#FF6A00"
                        return "#00C853"
                    }
                    border.width: 1

                    Text {
                        id: stateLabel
                        anchors.centerIn: parent
                        text: {
                            if (!planMasterController) return "PLANNER READY"
                            if (planMasterController.syncInProgress) return "UPLOADING..."
                            if (planMasterController.dirty) return "UNSAVED CHANGES"
                            if (missionController && missionController.visualItems && missionController.visualItems.count > 1) return "READY TO UPLOAD"
                            return "PLAN SYNCED"
                        }
                        font.family: "Inter"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (planMasterController && planMasterController.syncInProgress) return "#FFC107"
                            if (planMasterController && planMasterController.dirty) return "#FF6A00"
                            return "#00C853"
                        }
                    }
                }
            }
        }
    }
}
