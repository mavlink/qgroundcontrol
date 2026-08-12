/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Window Control Buttons
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

RowLayout {
    id: winButtonsRoot
    spacing: 4

    property color hoverColor: ThemeController.isDark ? "#2B3542" : "#E2E8F0"
    property color closeHoverColor: "#E53935"

    // 1. MINIMIZE BUTTON
    Rectangle {
        id: minBtn
        implicitWidth: 34
        implicitHeight: 30
        radius: 6
        color: minMouse.containsMouse ? winButtonsRoot.hoverColor : "transparent"

        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            anchors.centerIn: parent
            text: "─"
            font.pixelSize: 11
            font.weight: Font.Bold
            color: minMouse.containsMouse ? ThemeController.textPrimary : ThemeController.textSecondary
        }

        MouseArea {
            id: minMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (Window.window) {
                    Window.window.showMinimized()
                }
            }
        }
    }

    // 2. MAXIMIZE / RESTORE BUTTON
    Rectangle {
        id: maxBtn
        implicitWidth: 34
        implicitHeight: 30
        radius: 6
        color: maxMouse.containsMouse ? winButtonsRoot.hoverColor : "transparent"

        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            anchors.centerIn: parent
            text: (Window.window && Window.window.visibility === Window.Maximized) ? "🗗" : "▢"
            font.pixelSize: 12
            font.weight: Font.Bold
            color: maxMouse.containsMouse ? ThemeController.textPrimary : ThemeController.textSecondary
        }

        MouseArea {
            id: maxMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (Window.window) {
                    if (Window.window.visibility === Window.Maximized) {
                        Window.window.showNormal()
                    } else {
                        Window.window.showMaximized()
                    }
                }
            }
        }
    }

    // 3. CLOSE BUTTON
    Rectangle {
        id: closeBtn
        implicitWidth: 34
        implicitHeight: 30
        radius: 6
        color: closeMouse.containsMouse ? winButtonsRoot.closeHoverColor : "transparent"

        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            anchors.centerIn: parent
            text: "✕"
            font.pixelSize: 13
            font.weight: Font.Bold
            color: closeMouse.containsMouse ? "#FFFFFF" : ThemeController.textSecondary
        }

        MouseArea {
            id: closeMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (typeof mainWindow !== "undefined" && mainWindow && mainWindow.finishCloseProcess) {
                    mainWindow.finishCloseProcess()
                } else if (Window.window) {
                    Window.window.close()
                }
            }
        }
    }
}
