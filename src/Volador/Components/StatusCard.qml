/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Subsystem Status Card
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Card {
    id: root
    implicitWidth: 200
    implicitHeight: 88

    property string title: "VEHICLE LINK"
    property string statusText: "OPERATIONAL"
    property color statusColor: ThemeController.success

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Metrics.spacingMd
        spacing: Metrics.spacingXs

        Text {
            text: root.title.toUpperCase()
            font.family: Typography.fontFamily
            font.pointSize: Typography.captionSize
            font.weight: Typography.weightMedium
            color: ThemeController.textSecondary
        }

        RowLayout {
            spacing: Metrics.spacingSm

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: root.statusColor
            }

            Text {
                text: root.statusText
                font.family: Typography.fontFamily
                font.pointSize: Typography.bodySize
                font.weight: Typography.weightBold
                color: ThemeController.textPrimary
            }
        }
    }
}
