/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Danger / Critical Action Button
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import "qrc:/qml/VoladorTheme"


Button {
    id: control
    implicitHeight: Metrics.minTouchHeight
    implicitWidth: Math.max(120, buttonRow.implicitWidth + (Metrics.spacingLg * 2))

    property string iconSource: ""

    background: Rectangle {
        implicitHeight: Metrics.minTouchHeight
        radius: Metrics.radiusMd
        color: control.down ? Qt.darker(ThemeController.danger, 1.2) :
               (control.hovered ? Qt.lighter(ThemeController.danger, 1.1) : ThemeController.danger)

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Row {
        id: buttonRow
        spacing: Metrics.spacingSm
        anchors.centerIn: parent

        Image {
            source: control.iconSource
            sourceSize.height: 20
            visible: control.iconSource.length > 0
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: control.text
            font.family: Typography.fontFamily
            font.pointSize: Typography.buttonSize
            font.weight: Typography.weightBold
            color: "#FFFFFF"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
