/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Notification Center Button
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: notifyBtnRoot

    property int unreadCount: 3
    property bool isActive: false
    signal clicked()

    implicitWidth: 34
    implicitHeight: 34
    radius: 17
    color: isActive ? (ThemeController.isDark ? "#2A3340" : "#E2E8F0") :
           (btnMouse.containsMouse ? (ThemeController.isDark ? "#222B36" : "#F1F5F9") : "transparent")
    border.color: isActive ? ThemeController.accent : (btnMouse.containsMouse ? ThemeController.border : "transparent")
    border.width: 1

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    Text {
        anchors.centerIn: parent
        text: "🔔"
        font.pixelSize: 14
        opacity: btnMouse.containsMouse || notifyBtnRoot.isActive ? 1.0 : 0.85
    }

    // Unread Badge Pill
    Rectangle {
        id: badgePill
        width: notifyBtnRoot.unreadCount > 9 ? 16 : 14
        height: 14
        radius: 7
        color: ThemeController.accent
        border.color: ThemeController.background
        border.width: 1.5
        anchors.top: parent.top
        anchors.topMargin: -1
        anchors.right: parent.right
        anchors.rightMargin: -1
        visible: notifyBtnRoot.unreadCount > 0

        Text {
            anchors.centerIn: parent
            text: notifyBtnRoot.unreadCount > 99 ? "99+" : notifyBtnRoot.unreadCount.toString()
            font.family: "Inter"
            font.pixelSize: 9
            font.weight: Font.Bold
            color: "#FFFFFF"
        }
    }

    MouseArea {
        id: btnMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: notifyBtnRoot.clicked()
    }
}
