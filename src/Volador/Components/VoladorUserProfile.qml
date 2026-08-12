/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - User Profile Widget & Dropdown
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: userProfileRoot

    property string username: (typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.currentUser) ? voladorAuth.currentUser : "Pilot Operator"
    property string userRole: (typeof voladorAuth !== "undefined" && voladorAuth && voladorAuth.userRole) ? voladorAuth.userRole : "Flight Lead"
    property bool menuOpen: false

    implicitHeight: 34
    implicitWidth: profileRow.implicitWidth + 20
    radius: 17
    color: profileMouse.containsMouse || menuOpen ? (ThemeController.isDark ? "#242D3A" : "#E2E8F0") : (ThemeController.isDark ? "#1B232D" : "#F3F5F7")
    border.color: profileMouse.containsMouse || menuOpen ? ThemeController.accent : ThemeController.border
    border.width: 1

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    RowLayout {
        id: profileRow
        anchors.centerIn: parent
        spacing: 8

        // Avatar Circle with Status Dot
        Item {
            implicitWidth: 24
            implicitHeight: 24

            Rectangle {
                anchors.fill: parent
                radius: 12
                color: ThemeController.accent
                Text {
                    anchors.centerIn: parent
                    text: userProfileRoot.username.length > 0 ? userProfileRoot.username.substring(0, 1).toUpperCase() : "P"
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                }
            }

            // Green Online Status Dot
            Rectangle {
                width: 7
                height: 7
                radius: 3.5
                color: ThemeController.success
                border.color: ThemeController.background
                border.width: 1
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        Column {
            Layout.alignment: Qt.AlignVCenter
            spacing: 0

            Text {
                text: userProfileRoot.username
                font.family: "Inter"
                font.pixelSize: 11
                font.weight: Font.Bold
                color: ThemeController.textPrimary
            }

            Text {
                text: userProfileRoot.userRole
                font.family: "Inter"
                font.pixelSize: 9
                color: ThemeController.accent
            }
        }

        Text {
            text: userProfileRoot.menuOpen ? "▲" : "▼"
            font.pixelSize: 8
            color: ThemeController.textSecondary
            Layout.alignment: Qt.AlignVCenter
        }
    }

    MouseArea {
        id: profileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: userProfileRoot.menuOpen = !userProfileRoot.menuOpen
    }

    // -------------------------------------------------------------------------
    // ANIMATED POPUP MENU DROPDOWN
    // -------------------------------------------------------------------------
    Popup {
        id: menuPopup
        x: userProfileRoot.width - width
        y: userProfileRoot.height + 6
        width: 220
        height: menuCol.implicitHeight + 16
        padding: 8
        visible: userProfileRoot.menuOpen
        onClosed: userProfileRoot.menuOpen = false

        background: Rectangle {
            color: ThemeController.isDark ? "#171F2A" : "#FFFFFF"
            border.color: ThemeController.border
            border.width: 1
            radius: 8

            // Ambient Shadow
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: ThemeController.isDark ? "#2C3847" : "#CBD5E1"
            }
        }

        ColumnLayout {
            id: menuCol
            anchors.fill: parent
            spacing: 4

            // User Info Header Card inside menu
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 44
                radius: 6
                color: ThemeController.isDark ? "#12171E" : "#F8FAFC"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: ThemeController.accent
                        Text {
                            anchors.centerIn: parent
                            text: userProfileRoot.username.substring(0, 1).toUpperCase()
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }

                    Column {
                        Layout.fillWidth: true
                        Text { text: userProfileRoot.username; font.family: "Inter"; font.pixelSize: 12; font.weight: Font.Bold; color: ThemeController.textPrimary }
                        Text { text: userProfileRoot.userRole + " • Online"; font.family: "Inter"; font.pixelSize: 10; color: ThemeController.success }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

            // Menu Items
            Repeater {
                model: [
                    { icon: "👤", label: "My Profile", action: "profile" },
                    { icon: "⚙️", label: "Preferences", action: "settings" },
                    { icon: "🔑", label: "Account Security", action: "account" },
                    { icon: "❓", label: "Help & Manuals", action: "help" },
                    { icon: "ℹ️", label: "About VGCS", action: "about" }
                ]

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: 4
                    color: itemM.containsMouse ? (ThemeController.isDark ? "#263242" : "#F1F5F9") : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        Text { text: modelData.icon; font.pixelSize: 12 }
                        Text {
                            text: modelData.label
                            font.family: "Inter"
                            font.pixelSize: 12
                            color: itemM.containsMouse ? ThemeController.accent : ThemeController.textPrimary
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: itemM
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            userProfileRoot.menuOpen = false
                            if (modelData.action === "about") {
                                if (typeof mainWindow !== "undefined" && mainWindow.showAboutDialog) {
                                    mainWindow.showAboutDialog()
                                }
                            } else if (modelData.action === "settings") {
                                if (typeof mainWindow !== "undefined" && mainWindow.showSettingsTool) {
                                    mainWindow.showSettingsTool()
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

            // Logout Item
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 34
                radius: 4
                color: logoutM.containsMouse ? ThemeController.danger : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Text { text: "🚪"; font.pixelSize: 13 }
                    Text {
                        text: "Logout Operator"
                        font.family: "Inter"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        color: logoutM.containsMouse ? "#FFFFFF" : ThemeController.danger
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: logoutM
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        userProfileRoot.menuOpen = false
                        if (typeof voladorAuth !== "undefined" && voladorAuth) {
                            voladorAuth.logout()
                        }
                    }
                }
            }
        }
    }
}
