/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Navigation Rail Item
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: navItemRoot

    property string iconSource: ""
    property string itemLabel: ""
    property string tooltipText: ""
    property bool isActive: false
    property int itemIndex: 0

    signal itemClicked(int index)

    width: parent ? parent.width - 12 : 64
    height: 56
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    radius: 6
    color: isActive ? Qt.rgba(1.0, 0.416, 0.0, 0.12) : (mouseArea.containsMouse ? (ThemeController.isDark ? "#1D2733" : "#F3F5F7") : "transparent")
    border.color: (navItemRoot.activeFocus && !mouseArea.containsMouse) ? ThemeController.accent : "transparent"
    border.width: 1

    Behavior on color {
        ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    // Left Aerospace Active Accent Bar
    Rectangle {
        id: activeBar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        width: 3.5
        radius: 2
        color: ThemeController.accent
        visible: navItemRoot.isActive
        opacity: navItemRoot.isActive ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 4

        QGCColoredImage {
            id: itemIcon
            anchors.horizontalCenter: parent.horizontalCenter
            source: navItemRoot.iconSource
            width: 22
            height: 22
            sourceSize.width: 44
            sourceSize.height: 44
            fillMode: Image.PreserveAspectFit
            color: navItemRoot.isActive ? ThemeController.accent : 
                   (mouseArea.containsMouse ? ThemeController.textPrimary : ThemeController.textSecondary)

            Behavior on color {
                ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
            }
        }

        Text {
            id: labelText
            anchors.horizontalCenter: parent.horizontalCenter
            text: navItemRoot.itemLabel
            font.family: "Inter"
            font.pixelSize: 9
            font.weight: navItemRoot.isActive ? Font.Bold : Font.DemiBold
            font.letterSpacing: 0.5
            color: navItemRoot.isActive ? ThemeController.accent : 
                   (mouseArea.containsMouse ? ThemeController.textPrimary : ThemeController.textSecondary)

            Behavior on color {
                ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
            }
        }
    }

    // Aerospace Dark Surface Tooltip
    ToolTip {
        id: navToolTip
        visible: mouseArea.containsMouse
        delay: 300
        timeout: 4000
        text: navItemRoot.tooltipText !== "" ? navItemRoot.tooltipText : navItemRoot.itemLabel
        x: navItemRoot.width + 12
        y: (navItemRoot.height - height) / 2

        contentItem: Text {
            text: navToolTip.text
            font.family: "Inter"
            font.pixelSize: 11
            font.weight: Font.Medium
            color: "#F5F7FA"
        }

        background: Rectangle {
            color: "#1D2733"
            border.color: "#2C3847"
            border.width: 1
            radius: 4
            implicitHeight: 26
            implicitWidth: contentItem.implicitWidth + 16
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            navItemRoot.forceActiveFocus()
            navItemRoot.itemClicked(navItemRoot.itemIndex)
        }
    }

    Accessible.role: Accessible.Button
    Accessible.name: navItemRoot.tooltipText !== "" ? navItemRoot.tooltipText : navItemRoot.itemLabel
    Accessible.description: navItemRoot.tooltipText

    Keys.onReturnPressed: navItemRoot.itemClicked(navItemRoot.itemIndex)
    Keys.onEnterPressed: navItemRoot.itemClicked(navItemRoot.itemIndex)
    Keys.onSpacePressed: navItemRoot.itemClicked(navItemRoot.itemIndex)
}
