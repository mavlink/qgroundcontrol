/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - System Info Card
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Card {
    id: root
    implicitWidth: 320
    implicitHeight: col.implicitHeight + (Metrics.spacingMd * 2)

    property string title: "System Notice"
    property string description: "All systems nominal."
    property string icon: "ℹ️"

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: Metrics.spacingMd
        spacing: Metrics.spacingSm

        RowLayout {
            spacing: Metrics.spacingSm
            Text {
                text: root.icon
                font.pixelSize: 18
            }
            Text {
                text: root.title
                font.family: Typography.fontFamily
                font.pointSize: Typography.bodySize
                font.weight: Typography.weightBold
                color: ThemeController.textPrimary
            }
        }

        Text {
            text: root.description
            font.family: Typography.fontFamily
            font.pointSize: Typography.captionSize
            color: ThemeController.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
