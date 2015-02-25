import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FirmwareUpgradeController 1.0

Rectangle {
    width: 600
    height: 600

    property var qgcPal: QGCPalette { colorGroupEnabled: true }
    property FirmwareUpgradeController controller: FirmwareUpgradeController {
        upgradeButton: upgradeButton
        progressBar: progressBar
        statusLog: statusTextArea
        firmwareType: FirmwareUpgradeController.StableFirmware
    }

    color: qgcPal.window

    Column {
        anchors.fill:parent

        Text {
            text: "FIRMWARE UPDATE"
            color: qgcPal.text
            font.pointSize: 20
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        Row {
            spacing: 10

            ListModel {
                id: firmwareItems
                ListElement {
                    text: qsTr("Standard Version (stable)");
                    firmwareType: FirmwareUpgradeController.StableFirmware
                }
                ListElement {
                    text: qsTr("Beta Testing (beta)");
                    firmwareType: FirmwareUpgradeController.BetaFirmware
                }
                ListElement {
                    text: qsTr("Developer Build (master)");
                    firmwareType: FirmwareUpgradeController.DeveloperFirmware
                }
                ListElement {
                    text: qsTr("Custom firmware file...");
                    firmwareType: FirmwareUpgradeController.CustomFirmware
                }
            }

            QGCComboBox {
                id: firmwareCombo
                width: 200
                height: upgradeButton.height
                model: firmwareItems
            }

            QGCButton {
                id: upgradeButton
                text: "UPGRADE"
                primary: true
                onClicked: {
                    controller.firmwareType = firmwareItems.get(firmwareCombo.currentIndex).firmwareType
                    controller.doFirmwareUpgrade();
                }
            }
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        ProgressBar {
            id: progressBar
            width: parent.width
        }

        TextArea {
            id: statusTextArea
            width: parent.width
            height: 300
            readOnly: true
            frameVisible: false
            style: TextAreaStyle {
                textColor: qgcPal.text
                backgroundColor: qgcPal.windowShade
            }
        }
    }
}
