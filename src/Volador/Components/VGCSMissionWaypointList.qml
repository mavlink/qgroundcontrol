/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Mission Waypoint Sequence List
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: waypointListRoot

    property var planMasterController: null
    property var missionController: planMasterController ? planMasterController.missionController : null
    property var visualItems: missionController ? missionController.visualItems : null

    property bool isCollapsed: false

    implicitWidth: isCollapsed ? 36 : 260
    implicitHeight: 340
    radius: 4
    color: "#E6151C24" // 90% dark glass
    border.color: "#2C3847"
    border.width: 1
    clip: true

    Behavior on implicitWidth {
        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Header Bar
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: "#151C24"
            border.color: "#2C3847"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 6

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: "#FF6A00"
                    visible: !waypointListRoot.isCollapsed
                }

                Text {
                    text: "WAYPOINTS"
                    font.family: "Inter"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.8
                    color: "#F5F7FA"
                    visible: !waypointListRoot.isCollapsed
                }

                Rectangle {
                    height: 18
                    implicitWidth: countText.implicitWidth + 8
                    radius: 9
                    color: "#1D2733"
                    border.color: "#2C3847"
                    border.width: 1
                    visible: !waypointListRoot.isCollapsed

                    Text {
                        id: countText
                        anchors.centerIn: parent
                        text: (visualItems && visualItems.count > 1) ? (visualItems.count - 1).toString() : "0"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: "#FF6A00"
                    }
                }

                Item { Layout.fillWidth: true }

                // Collapse/Expand Toggle Button
                Rectangle {
                    width: 24
                    height: 24
                    radius: 3
                    color: toggleMouse.containsMouse ? "#2C3847" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: waypointListRoot.isCollapsed ? "▶" : "◀"
                        font.pixelSize: 10
                        color: "#9BA8B5"
                    }

                    MouseArea {
                        id: toggleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: waypointListRoot.isCollapsed = !waypointListRoot.isCollapsed
                    }

                    ToolTip.visible: toggleMouse.containsMouse
                    ToolTip.text: waypointListRoot.isCollapsed ? "Expand Waypoints List" : "Collapse Waypoints List"
                    ToolTip.delay: 400
                }
            }
        }

        // 2. Waypoints List / Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !waypointListRoot.isCollapsed

            // Empty state
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 12
                visible: !visualItems || visualItems.count <= 1
                spacing: 6

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "NO WAYPOINTS"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: "#9BA8B5"
                }

                Text {
                    Layout.fillWidth: true
                    text: "Use the toolbar to insert waypoints or generate survey patterns."
                    font.family: "Inter"
                    font.pixelSize: 8
                    color: "#64748B"
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Scrollable ListView
            ListView {
                id: wpListView
                anchors.fill: parent
                anchors.margins: 4
                spacing: 3
                clip: true
                model: visualItems
                boundsBehavior: Flickable.StopAtBounds
                visible: visualItems && visualItems.count > 0

                delegate: Rectangle {
                    id: itemDelegate
                    readonly property var missionItem: (typeof object !== "undefined") ? object : null
                    width: wpListView.width
                    height: 38
                    radius: 3
                    color: {
                        if (missionItem && missionItem.isCurrentItem) return "#253342"
                        if (itemMouse.containsMouse) return "#1D2733"
                        return "#151C24"
                    }
                    border.color: (missionItem && missionItem.isCurrentItem) ? "#FF6A00" : (itemMouse.containsMouse ? "#3A4B5C" : "#222D39")
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        // Sequence Number Badge
                        Rectangle {
                            width: 22
                            height: 22
                            radius: 3
                            color: {
                                if (index === 0) return "#1D2733"
                                if (missionItem && missionItem.isCurrentItem) return "#FF6A00"
                                return "#2C3847"
                            }

                            Text {
                                anchors.centerIn: parent
                                text: index === 0 ? "H" : (missionItem ? missionItem.sequenceNumber.toString() : "")
                                font.family: "JetBrains Mono"
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                color: {
                                    if (index === 0) return "#00C853"
                                    if (missionItem && missionItem.isCurrentItem) return "#0A0F14"
                                    return "#F5F7FA"
                                }
                            }
                        }

                        // Command & Info
                        Column {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: index === 0 ? "PLANNED HOME" : ((missionItem && missionItem.commandName) ? missionItem.commandName.toUpperCase() : "WAYPOINT")
                                font.family: "Inter"
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                color: (missionItem && missionItem.isCurrentItem) ? "#F5F7FA" : "#D0D7DE"
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            RowLayout {
                                spacing: 6
                                Text {
                                    text: {
                                        if (index === 0) return "ORIGIN"
                                        var alt = 0
                                        if (missionItem && missionItem.altitude && missionItem.altitude.value !== undefined) {
                                            alt = missionItem.altitude.value
                                        } else if (missionItem && missionItem.amslEntryAlt !== undefined && !isNaN(missionItem.amslEntryAlt)) {
                                            alt = missionItem.amslEntryAlt
                                        }
                                        return alt.toFixed(0) + "m ALT"
                                    }
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 8
                                    color: "#00C853"
                                }

                                Text {
                                    text: "•"
                                    font.pixelSize: 8
                                    color: "#64748B"
                                    visible: index > 0 && missionItem !== null && missionItem.distance > 0
                                }

                                Text {
                                    text: {
                                        if (index === 0 || !missionItem || !missionItem.distance) return ""
                                        var d = missionItem.distance
                                        return d >= 1000 ? (d / 1000.0).toFixed(1) + "km" : d.toFixed(0) + "m"
                                    }
                                    font.family: "JetBrains Mono"
                                    font.pixelSize: 8
                                    color: "#9BA8B5"
                                    visible: index > 0 && missionItem !== null && missionItem.distance > 0
                                }
                            }
                        }

                        // Delete button (on hover or current)
                        Rectangle {
                            width: 18
                            height: 18
                            radius: 2
                            color: delBtnMouse.containsMouse ? "#D32F2F" : "transparent"
                            visible: index > 0 && (itemMouse.containsMouse || (missionItem && missionItem.isCurrentItem))

                            Text {
                                anchors.centerIn: parent
                                text: "✕"
                                font.pixelSize: 8
                                font.weight: Font.Bold
                                color: delBtnMouse.containsMouse ? "#FFFFFF" : "#9BA8B5"
                            }

                            MouseArea {
                                id: delBtnMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (missionController) {
                                        missionController.removeVisualItem(index)
                                    }
                                }
                            }

                            ToolTip.visible: delBtnMouse.containsMouse
                            ToolTip.text: "Delete waypoint"
                            ToolTip.delay: 300
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (missionController && missionItem) {
                                missionController.setCurrentPlanViewSeqNum(missionItem.sequenceNumber, true)
                            }
                        }
                    }
                }
            }
        }
    }
}
