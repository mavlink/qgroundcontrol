/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Secondary Industrial Button
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import "qrc:/qml/VoladorTheme"


Button {
    id: control
    implicitHeight: Metrics.minTouchHeight
    implicitWidth: Math.max(100, buttonRow.implicitWidth + (Metrics.spacingLg * 2))

    property string iconSource: ""

    background: Rectangle {
        implicitHeight: Metrics.minTouchHeight
        radius: Metrics.radiusMd
        color: control.down ? Qt.darker(ThemeController.panel, 1.1) :
               (control.hovered ? ThemeController.panel : ThemeController.cards)
        border.color: ThemeController.border
        border.width: 1

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
            font.weight: Typography.weightSemiBold
            color: ThemeController.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
