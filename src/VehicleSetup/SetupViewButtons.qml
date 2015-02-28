import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtGraphicalEffects 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    id: topLevel

    QGCPalette { id: palette; colorGroupEnabled: true }
    color: palette.window

    ExclusiveGroup { id: setupButtonGroup }

    Component {
        id: disconnectedButtons

        Column {
            anchors.fill: parent

            SubMenuButton {
                id: firmwareButton; objectName: "firmwareButton"
                width: parent.width
                text: "FIRMWARE"
                imageResource: "FirmwareUpgradeIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.firmwareButtonClicked()
            }

            Item { width: parent.width; height: 10 }    // spacer

            QGCLabel {
                width: parent.width
                text: "You must be connected to your board to use all available setup options."
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Component {
        id: connectedButtons

        Column {
            anchors.fill: parent

            SubMenuButton {
                id: summaryButton; objectName: "summaryButton"
                width: parent.width
                text: "SUMMARY"
                imageResource: "VehicleSummaryIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.summaryButtonClicked()
            }

            SubMenuButton {
                id: firmwareButton; objectName: "firmwareButton"
                width: parent.width
                text: "FIRMWARE"
                imageResource: "FirmwareUpgradeIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.firmwareButtonClicked()
            }

            Repeater {
                model: autopilot.components

                SubMenuButton {
                    width: parent.width
                    text: modelData.name.toUpperCase()
                    imageResource: modelData.iconResource
                    setupComplete: modelData.setupComplete
                    exclusiveGroup: setupButtonGroup
                    onClicked: controller.setupButtonClicked(modelData)
                }
            }

            SubMenuButton {
                width: parent.width
                text: "PARAMETERS"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.parametersButtonClicked()
            }
        }
    }


    Loader {
        anchors.fill: parent
        sourceComponent: autopilot ? connectedButtons : disconnectedButtons
    }
}
