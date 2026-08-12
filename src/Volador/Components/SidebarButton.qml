/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Sidebar Navigation Rail Button
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Rectangle {
    id: root
    implicitWidth: parent ? parent.width : 220
    implicitHeight: Metrics.minTouchHeight
    color: isActive ? Qt.alpha(ThemeController.accent, 0.12) :
           (mouseArea.containsMouse ? ThemeController.panel : "transparent")
    radius: Metrics.radiusMd

    property string text: "Dashboard"
    property string iconSource: ""
    property bool isActive: false
    property bool isCompact: false
    signal clicked()

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    // Left Active Indicator Bar
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        radius: 2
        color: ThemeController.accent
        visible: root.isActive
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Metrics.spacingLg
        anchors.rightMargin: Metrics.spacingLg
        spacing: Metrics.spacingMd

        Image {
            source: root.iconSource
            sourceSize.height: 22
            sourceSize.width: 22
            visible: root.iconSource.length > 0
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.text
            font.family: Typography.fontFamily
            font.pointSize: Typography.bodySize
            font.weight: root.isActive ? Typography.weightBold : Typography.weightMedium
            color: root.isActive ? ThemeController.accent : ThemeController.textPrimary
            visible: !root.isCompact
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
