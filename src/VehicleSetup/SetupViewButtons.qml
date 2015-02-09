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

    signal firmwareButtonClicked;
    signal summaryButtonClicked;
    signal parametersButtonClicked;
    signal setupButtonClicked(variant component);

    ExclusiveGroup { id: setupButtonGroup }

    Component {
        id: disconnectedButtons

        Column {
            spacing: 10

            SetupButton {
                id: firmwareButton; objectName: "firmwareButton"
                width: parent.width
                text: "FIRMWARE"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: topLevel.firmwareButtonClicked()
            }
        }
    }

    Component {
        id: connectedButtons

        Column {
            spacing: 10

            SetupButton {
                id: summaryButton; objectName: "summaryButton"
                width: parent.width
                text: "SUMMARY"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: topLevel.summaryButtonClicked()
            }

            SetupButton {
                id: firmwareButton; objectName: "firmwareButton"
                width: parent.width
                text: "FIRMWARE"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: topLevel.firmwareButtonClicked()
            }

            Repeater {
                model: autopilot.components

                SetupButton {
                    width: parent.width
                    text: modelData.name.toUpperCase()
                    setupComplete: modelData.setupComplete
                    exclusiveGroup: setupButtonGroup
                    onClicked: topLevel.setupButtonClicked(modelData)
                }
            }

            SetupButton {
                width: parent.width
                text: "PARAMETERS"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                onClicked: topLevel.parametersButtonClicked()
            }
        }
    }


    Loader {
        anchors.fill: parent
        sourceComponent: autopilot ? connectedButtons : disconnectedButtons
    }
}
