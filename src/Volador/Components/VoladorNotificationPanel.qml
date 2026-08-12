/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Slide-out Notification Panel
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: panelRoot

    property bool isOpen: false
    property string activeFilter: "All"
    signal closeRequested()

    width: 380
    color: ThemeController.isDark ? "#12171E" : "#FFFFFF"
    border.color: ThemeController.border
    border.width: 1
    clip: true

    // Slide-out Animation
    x: isOpen ? parent.width - width : parent.width

    Behavior on x {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }

    // Shadow Edge
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.left
        width: 12
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#40000000" }
        }
        visible: panelRoot.isOpen
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. PANEL HEADER
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 56
            color: ThemeController.isDark ? "#171F2A" : "#F8FAFC"
            border.color: ThemeController.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "🔔"
                    font.pixelSize: 16
                }

                Text {
                    text: "NOTIFICATIONS"
                    font.family: "Inter"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: ThemeController.textPrimary
                    Layout.fillWidth: true
                }

                Rectangle {
                    implicitWidth: 80
                    implicitHeight: 24
                    radius: 12
                    color: markMouse.containsMouse ? ThemeController.accent : (ThemeController.isDark ? "#232D3B" : "#E2E8F0")

                    Text {
                        anchors.centerIn: parent
                        text: "Clear All"
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: markMouse.containsMouse ? "#FFFFFF" : ThemeController.textSecondary
                    }

                    MouseArea {
                        id: markMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: notifyModel.clear()
                    }
                }

                // Close Panel Button
                Rectangle {
                    implicitWidth: 26
                    implicitHeight: 26
                    radius: 13
                    color: closeMouse.containsMouse ? (ThemeController.isDark ? "#2C3847" : "#CBD5E1") : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: ThemeController.textSecondary
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: panelRoot.closeRequested()
                    }
                }
            }
        }

        // 2. CATEGORY FILTER TABS
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            color: ThemeController.isDark ? "#151C24" : "#F1F5F9"
            border.color: ThemeController.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                Repeater {
                    model: ["All", "Critical", "Warnings", "Info", "Mission"]

                    Rectangle {
                        implicitHeight: 26
                        implicitWidth: filterText.implicitWidth + 16
                        radius: 13
                        color: panelRoot.activeFilter === modelData ? ThemeController.accent : (fMouse.containsMouse ? (ThemeController.isDark ? "#2A3544" : "#E2E8F0") : "transparent")

                        Text {
                            id: filterText
                            anchors.centerIn: parent
                            text: modelData
                            font.family: "Inter"
                            font.pixelSize: 11
                            font.weight: panelRoot.activeFilter === modelData ? Font.Bold : Font.Normal
                            color: panelRoot.activeFilter === modelData ? "#FFFFFF" : ThemeController.textSecondary
                        }

                        MouseArea {
                            id: fMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: panelRoot.activeFilter = modelData
                        }
                    }
                }
            }
        }

        // 3. NOTIFICATIONS LIST
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: notifyList
                anchors.fill: parent
                spacing: 8
                topMargin: 12
                bottomMargin: 12

                model: ListModel {
                    id: notifyModel
                    ListElement {
                        type: "Critical"
                        iconStr: "🚨"
                        titleStr: "Geofence Breach Warning"
                        descStr: "Vehicle Drone #01 approaching 200m perimeter border limit."
                        timeStr: "2 mins ago"
                        accentColor: "#F44336"
                    }
                    ListElement {
                        type: "Warnings"
                        iconStr: "⚠️"
                        titleStr: "High Wind Speed Alarm"
                        descStr: "Microclimate WX sensor recorded 28.4 km/h gust near Zone B."
                        timeStr: "8 mins ago"
                        accentColor: "#FFC107"
                    }
                    ListElement {
                        type: "Mission"
                        iconStr: "🎯"
                        titleStr: "Waypoint 12 Reached"
                        descStr: "Autonomous Flight Plan #04 executing Grid Survey scan."
                        timeStr: "15 mins ago"
                        accentColor: "#00C853"
                    }
                    ListElement {
                        type: "Info"
                        iconStr: "📡"
                        titleStr: "MAVLink Stack Synchronized"
                        descStr: "MAVLink v2.0 telemetry protocol linked with Flight Controller."
                        timeStr: "24 mins ago"
                        accentColor: "#2196F3"
                    }
                    ListElement {
                        type: "System"
                        iconStr: "⚙️"
                        titleStr: "VGCS Engine Core Ready"
                        descStr: "Volador Ground Control Station v2.0.0-alpha.1 initialized."
                        timeStr: "1 hour ago"
                        accentColor: "#FF6A00"
                    }
                }

                delegate: Rectangle {
                    width: notifyList.width - 24
                    anchors.horizontalCenter: parent.horizontalCenter
                    implicitHeight: 74
                    radius: 8
                    color: itemMouse.containsMouse ? (ThemeController.isDark ? "#1E2734" : "#F8FAFC") : (ThemeController.isDark ? "#171F2A" : "#F1F5F9")
                    border.color: itemMouse.containsMouse ? accentColor : ThemeController.border
                    border.width: 1

                    visible: panelRoot.activeFilter === "All" || panelRoot.activeFilter === type

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12

                        // Left Accent Bar
                        Rectangle {
                            implicitWidth: 4
                            Layout.fillHeight: true
                            radius: 2
                            color: accentColor
                        }

                        Text {
                            text: iconStr
                            font.pixelSize: 18
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: titleStr
                                    font.family: "Inter"
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: ThemeController.textPrimary
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: timeStr
                                    font.family: "Inter"
                                    font.pixelSize: 10
                                    color: ThemeController.textSecondary
                                }
                            }

                            Text {
                                text: descStr
                                font.family: "Inter"
                                font.pixelSize: 11
                                color: ThemeController.textSecondary
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }
        }
    }
}
