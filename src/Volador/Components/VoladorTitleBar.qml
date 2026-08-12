/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Enterprise Title Bar Shell
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: titleBarRoot

    implicitHeight: 52
    color: ThemeController.isDark ? "#12171F" : "#F8FAFC"
    border.color: ThemeController.border
    border.width: 1

    property alias notificationPanel: notifyPanel

    // -------------------------------------------------------------------------
    // WINDOW DRAG MOUSE AREA FOR FRAMELESS WINDOW MOVEMENT
    // -------------------------------------------------------------------------
    MouseArea {
        id: dragArea
        anchors.fill: parent
        z: -1
        property point startPoint: "0,0"

        onPressed: (mouse) => {
            startPoint = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: (mouse) => {
            if (pressed && Window.window) {
                var deltaX = mouse.x - startPoint.x
                var deltaY = mouse.y - startPoint.y
                Window.window.x += deltaX
                Window.window.y += deltaY
            }
        }

        onDoubleClicked: {
            if (Window.window) {
                if (Window.window.visibility === Window.Maximized) {
                    Window.window.showNormal()
                } else {
                    Window.window.showMaximized()
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        spacing: 12

        // ---------------------------------------------------------------------
        // 1. LEFT SIDE: BRAND EMBLEM & VGCS TITLE
        // ---------------------------------------------------------------------
        RowLayout {
            spacing: 10

            Image {
                source: "qrc:/Volador/Assets/Logos/volador_compact.png"
                implicitWidth: 30
                implicitHeight: 30
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                mipmap: true
            }

            Column {
                Layout.alignment: Qt.AlignVCenter
                spacing: 0

                Text {
                    text: "VGCS"
                    font.family: "Inter"
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    color: ThemeController.textPrimary
                }

                Text {
                    text: "COMMAND CENTER"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    color: ThemeController.accent
                }
            }
        }

        Rectangle {
            implicitWidth: 1
            implicitHeight: 22
            color: ThemeController.border
        }

        Item { Layout.fillWidth: true } // Left Spacer

        // ---------------------------------------------------------------------
        // 2. CENTER: GLOBAL SEARCH BAR
        // ---------------------------------------------------------------------
        VoladorSearchBar {
            id: globalSearch
            Layout.alignment: Qt.AlignVCenter
            onSearchSubmitted: (query) => {
                console.log("Global search submitted:", query)
            }
        }

        Item { Layout.fillWidth: true } // Right Spacer

        // ---------------------------------------------------------------------
        // 3. RIGHT SIDE: NOTIFICATIONS, THEME TOGGLE, USER PROFILE, CONTROLS
        // ---------------------------------------------------------------------
        RowLayout {
            spacing: 8

            // Notification Button
            VoladorNotificationButton {
                id: notifyBtn
                isActive: notifyPanel.isOpen
                onClicked: notifyPanel.isOpen = !notifyPanel.isOpen
            }

            // Theme Toggle Switch / Button
            Rectangle {
                implicitWidth: 34
                implicitHeight: 34
                radius: 17
                color: themeMouse.containsMouse ? (ThemeController.isDark ? "#242D3A" : "#E2E8F0") : "transparent"
                border.color: themeMouse.containsMouse ? ThemeController.border : "transparent"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: ThemeController.isDark ? "☀️" : "🌙"
                    font.pixelSize: 14
                }

                MouseArea {
                    id: themeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: ThemeController.toggleTheme()
                }
            }

            // User Profile Widget
            VoladorUserProfile {
                id: userProfileWidget
            }

            Rectangle {
                implicitWidth: 1
                implicitHeight: 22
                color: ThemeController.border
            }

            // Window Control Buttons (Minimize, Maximize, Close)
            VoladorWindowButtons {
                id: windowButtons
            }
        }
    }

    // -------------------------------------------------------------------------
    // SLIDE-OUT NOTIFICATION PANEL
    // -------------------------------------------------------------------------
    VoladorNotificationPanel {
        id: notifyPanel
        parent: Overlay.overlay ? Overlay.overlay : titleBarRoot.Window.window.contentItem
        anchors.top: parent ? parent.top : undefined
        anchors.bottom: parent ? parent.bottom : undefined
        z: 9999
        onCloseRequested: notifyPanel.isOpen = false
    }
}
