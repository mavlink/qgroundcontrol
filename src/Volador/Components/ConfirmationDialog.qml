/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Confirmation Dialog
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/qml/VoladorTheme"


Dialog {
    id: dialog
    modal: true
    anchors.centerIn: parent
    implicitWidth: 420
    implicitHeight: col.implicitHeight + (Metrics.spacingLg * 2)

    property string titleText: "Confirm Action"
    property string messageText: "Are you sure you want to proceed?"
    property bool isDangerAction: false
    signal confirmed()

    background: Card {
        color: ThemeController.cards
        border.color: ThemeController.border
    }

    contentItem: ColumnLayout {
        id: col
        spacing: Metrics.spacingLg

        Text {
            text: dialog.titleText
            font.family: Typography.fontFamily
            font.pointSize: Typography.titleSize
            font.weight: Typography.weightBold
            color: dialog.isDangerAction ? ThemeController.danger : ThemeController.textPrimary
        }

        Text {
            text: dialog.messageText
            font.family: Typography.fontFamily
            font.pointSize: Typography.bodySize
            color: ThemeController.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Metrics.spacingMd
            Layout.alignment: Qt.AlignRight

            SecondaryButton {
                text: "Cancel"
                onClicked: dialog.close()
            }

            PrimaryButton {
                visible: !dialog.isDangerAction
                text: "Confirm"
                onClicked: {
                    dialog.confirmed()
                    dialog.close()
                }
            }

            DangerButton {
                visible: dialog.isDangerAction
                text: "Execute"
                onClicked: {
                    dialog.confirmed()
                    dialog.close()
                }
            }
        }
    }
}
