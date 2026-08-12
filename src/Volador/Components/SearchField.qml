/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Industrial Search Input Field
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Rectangle {
    id: root
    implicitWidth: 260
    implicitHeight: Metrics.minTouchHeight
    radius: Metrics.radiusMd
    color: ThemeController.panel
    border.color: input.activeFocus ? ThemeController.accent : ThemeController.border
    border.width: 1

    property alias text: input.text
    property string placeholderText: "Search settings..."

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Metrics.spacingMd
        anchors.rightMargin: Metrics.spacingMd
        spacing: Metrics.spacingSm

        Text {
            text: "🔍"
            font.pixelSize: 14
        }

        TextField {
            id: input
            Layout.fillWidth: true
            placeholderText: root.placeholderText
            placeholderTextColor: ThemeController.textSecondary
            font.family: Typography.fontFamily
            font.pointSize: Typography.bodySize
            color: ThemeController.textPrimary
            background: null
        }

        Text {
            text: "✖"
            font.pixelSize: 12
            color: ThemeController.textSecondary
            visible: input.text.length > 0
            MouseArea {
                anchors.fill: parent
                onClicked: input.text = ""
            }
        }
    }
}
