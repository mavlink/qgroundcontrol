/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Status Card
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: statusCardRoot

    property string categoryLabel: ""
    property string primaryValue: ""
    property string secondaryValue: ""
    property color statusColor: "#64748B"
    property string iconSource: ""

    implicitHeight: 52
    Layout.fillWidth: true
    Layout.minimumWidth: 100
    Layout.preferredHeight: 52
    radius: 6
    color: cardMouse.containsMouse ? "#1D2733" : "#151C24"
    border.color: cardMouse.containsMouse ? "#3B4A5D" : "#2C3847"
    border.width: 1

    Behavior on color {
        ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
    }
    Behavior on border.color {
        ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    // Left Colored Status Indicator Bar
    Rectangle {
        id: indicatorBar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        width: 3
        radius: 1.5
        color: statusCardRoot.statusColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 10
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 2

        // Top Row: Category + Status Dot
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: statusCardRoot.categoryLabel
                font.family: "Inter"
                font.pixelSize: 9
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
                color: ThemeController.textSecondary
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: statusCardRoot.statusColor
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // Bottom Row: Primary Value (JetBrains Mono) + Secondary Value
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: statusCardRoot.primaryValue !== "" ? statusCardRoot.primaryValue : "N/A"
                font.family: "JetBrains Mono"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: ThemeController.textPrimary
                Layout.preferredWidth: implicitWidth
                Layout.maximumWidth: statusCardRoot.secondaryValue !== "" ? (statusCardRoot.width - 24) * 0.58 : (statusCardRoot.width - 24)
                elide: Text.ElideRight
            }

            Text {
                text: statusCardRoot.secondaryValue
                font.family: "Inter"
                font.pixelSize: 9
                font.weight: Font.Normal
                color: ThemeController.textSecondary
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                visible: statusCardRoot.secondaryValue !== ""
            }
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
    }
}
