import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0
import QtGraphicalEffects 1.0

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    id: topLevel

    QGCPalette { id: palette; colorGroup: QGCPalette.Active }
    color: palette.window

    ExclusiveGroup { id: setupButtonGroup }

    Component {
        id: disconnectedButtons

        Column {
            anchors.fill: parent

            SetupButton {
                id: firmwareButton; objectName: "firmwareButton"
                width: parent.width
                text: "FIRMWARE"
                imageResource: "FirmwareUpgradeIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.firmwareButtonClicked()
            }
        }
    }

    Component {
        id: connectedButtons

        Column {
            anchors.fill: parent

            SetupButton {
                id: summaryButton; objectName: "summaryButton"
                width: parent.width
                text: "SUMMARY"
                imageResource: "VehicleSummaryIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: controller.summaryButtonClicked()
            }

            SetupButton {
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

                SetupButton {
                    width: parent.width
                    text: modelData.name.toUpperCase()
                    imageResource: modelData.iconResource
                    setupComplete: modelData.setupComplete
                    exclusiveGroup: setupButtonGroup
                    onClicked: controller.setupButtonClicked(modelData)
                }
            }

            SetupButton {
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
