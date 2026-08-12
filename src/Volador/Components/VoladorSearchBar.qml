/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Global Search Component
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: searchRoot

    property alias text: searchInput.text
    property string placeholderText: "Search missions, vehicles, settings..."
    signal searchSubmitted(string query)
    signal searchCleared()

    implicitHeight: 34
    implicitWidth: searchInput.activeFocus ? 340 : 230
    radius: 17
    color: searchInput.activeFocus ? (ThemeController.isDark ? "#242E3B" : "#FFFFFF") : (ThemeController.isDark ? "#1D2733" : "#F3F5F7")
    border.color: searchInput.activeFocus ? ThemeController.accent : ThemeController.border
    border.width: 1

    Behavior on implicitWidth {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 10
        spacing: 8

        Text {
            text: "🔍"
            font.pixelSize: 12
            color: searchInput.activeFocus ? ThemeController.accent : ThemeController.textSecondary
            Layout.alignment: Qt.AlignVCenter
        }

        TextInput {
            id: searchInput
            Layout.fillWidth: true
            font.family: "Inter"
            font.pixelSize: 12
            color: ThemeController.textPrimary
            clip: true
            selectByMouse: true

            Text {
                text: searchRoot.placeholderText
                font.family: "Inter"
                font.pixelSize: 12
                color: ThemeController.textSecondary
                visible: !searchInput.text && !searchInput.activeFocus
                anchors.verticalCenter: parent.verticalCenter
            }

            onAccepted: {
                searchRoot.searchSubmitted(searchInput.text)
            }
        }

        // Clear text button
        Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            radius: 8
            color: clearMouse.containsMouse ? ThemeController.accent : ThemeController.border
            visible: searchInput.text.length > 0
            Layout.alignment: Qt.AlignVCenter

            Text {
                anchors.centerIn: parent
                text: "✕"
                font.pixelSize: 9
                font.weight: Font.Bold
                color: clearMouse.containsMouse ? "#FFFFFF" : ThemeController.textSecondary
            }

            MouseArea {
                id: clearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    searchInput.text = ""
                    searchRoot.searchCleared()
                }
            }
        }
    }
}
