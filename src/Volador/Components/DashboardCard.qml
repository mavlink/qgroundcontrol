/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Dashboard Widget Card
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import "qrc:/qml/VoladorTheme"

Card {
    id: dashCard

    property string title: ""
    property string icon: ""
    property string badgeText: ""
    property color badgeColor: ThemeController.accent

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Card Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: dashCard.title !== ""

            Text {
                text: dashCard.icon
                font.pixelSize: 16
                visible: dashCard.icon !== ""
            }

            Text {
                text: dashCard.title
                font.family: "Inter"
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: ThemeController.textPrimary
                Layout.fillWidth: true
            }

            Rectangle {
                visible: dashCard.badgeText !== ""
                height: 20
                implicitWidth: badgeLabel.implicitWidth + 12
                radius: 10
                color: dashCard.badgeColor

                Text {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text: dashCard.badgeText
                    font.family: "Inter"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                }
            }
        }

        // Card Content Slot
        Item {
            id: contentSlot
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
