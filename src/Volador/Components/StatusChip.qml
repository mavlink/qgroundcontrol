/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Status Indicator Chip / Pill
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Rectangle {
    id: chip
    implicitWidth: row.implicitWidth + (Metrics.spacingMd * 2)
    implicitHeight: 28
    radius: 14
    color: Qt.alpha(chipColor, 0.15)
    border.color: chipColor
    border.width: 1

    property string text: "READY"
    property color chipColor: ThemeController.success

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: Metrics.spacingSm

        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: chip.chipColor
        }

        Text {
            text: chip.text.toUpperCase()
            font.family: Typography.fontFamily
            font.pointSize: Typography.captionSize
            font.weight: Typography.weightBold
            color: ThemeController.textPrimary
        }
    }
}
