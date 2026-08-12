/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Telemetry Metric Display Card
 *
 * Direct Sunlight Readable 18pt Telemetry Metrics
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Card {
    id: root
    implicitWidth: 160
    implicitHeight: 76

    property string label: "ALTITUDE"
    property string value: "N/A"
    property string unit: "m"
    property color valueColor: ThemeController.textPrimary

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Metrics.spacingMd
        spacing: Metrics.spacingXs

        Text {
            text: root.label.toUpperCase()
            font.family: Typography.fontFamily
            font.pointSize: Typography.captionSize
            font.weight: Typography.weightMedium
            color: ThemeController.textSecondary
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Metrics.spacingXs
            Layout.fillWidth: true

            Text {
                text: root.value
                font.family: Typography.fontFamily
                font.pointSize: Typography.telemetrySize
                font.weight: Typography.weightBold
                color: root.valueColor
            }

            Text {
                text: root.unit
                font.family: Typography.fontFamily
                font.pointSize: Typography.bodySize
                font.weight: Typography.weightMedium
                color: ThemeController.textSecondary
                Layout.alignment: Qt.AlignBottom
            }
        }
    }
}
