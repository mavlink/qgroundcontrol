/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Day / Night Theme Toggle Switch
 *
 ****************************************************************************/

import QtQuick

import "qrc:/qml/VoladorTheme"


Rectangle {
    id: root
    implicitWidth: 64
    implicitHeight: 32
    radius: 16
    color: ThemeController.panel
    border.color: ThemeController.border
    border.width: 1

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: ThemeController.toggleTheme()
    }

    Rectangle {
        id: knob
        width: 24
        height: 24
        radius: 12
        anchors.verticalCenter: parent.verticalCenter
        x: ThemeController.isDarkMode ? root.width - width - 4 : 4
        color: ThemeController.accent

        Behavior on x {
            NumberAnimation { duration: 150; easing.type: Easing.InOutQuad }
        }

        Text {
            anchors.centerIn: parent
            text: ThemeController.isDarkMode ? "🌙" : "☀️"
            font.pixelSize: 12
        }
    }
}
