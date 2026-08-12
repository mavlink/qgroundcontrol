/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Loading & Busy Overlay
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Rectangle {
    id: overlay
    anchors.fill: parent
    color: Qt.alpha(ThemeController.background, 0.85)
    visible: active

    property bool active: false
    property string statusText: "Processing..."

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Metrics.spacingLg

        BusyIndicator {
            running: overlay.active
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: overlay.statusText
            font.family: Typography.fontFamily
            font.pointSize: Typography.titleSize
            font.weight: Typography.weightSemiBold
            color: ThemeController.textPrimary
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
