import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FirmwareUpgradeController 1.0

Rectangle {
    width: 600
    height: 600

    property var qgcPal: QGCPalette { colorGroup: QGCPalette.Active }
    property FirmwareUpgradeController controller: FirmwareUpgradeController {
        upgradeButton: upgradeButton
        statusLog: statusTextArea
        firmwareType: FirmwareUpgradeController.StableFirmware
    }

    color: qgcPal.window

    Column {
        anchors.fill:parent

        Text {
            text: "FIRMWARE UPDATE"
            color: qgcPal.windowText
            font.pointSize: 20
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        ExclusiveGroup { id: firmwareGroup }

        QGCRadioButton {
            id: stableFirwareRadio
            exclusiveGroup: firmwareGroup
            text: qsTr("Standard Version (stable)")
            checked: true
            enabled: upgradeButton.enabled
            onClicked: {
                if (checked)
                    controller.firmwareType = FirmwareUpgradeController.StableFirmware
            }
        }
        QGCRadioButton {
            id: betaFirwareRadio
            exclusiveGroup: firmwareGroup
            text: qsTr("Beta Testing (beta)")
            enabled: upgradeButton.enabled
            onClicked: { if (checked) controller.firmwareType = FirmwareUpgradeController.BetaFirmware }
        }
        QGCRadioButton {
            id: devloperFirwareRadio
            exclusiveGroup: firmwareGroup
            text: qsTr("Developer Build (master)")
            enabled: upgradeButton.enabled
            onClicked: { if (checked) controller.firmwareType = FirmwareUpgradeController.DeveloperFirmware }
        }
        QGCRadioButton {
            id: customFirwareRadio
            exclusiveGroup: firmwareGroup
            text: qsTr("Custom firmware file...")
            enabled: upgradeButton.enabled
            onClicked: { if (checked) controller.firmwareType = FirmwareUpgradeController.CustomFirmware }
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        QGCButton {
            id: upgradeButton
            text: "UPGRADE"
            onClicked: {
                controller.doFirmwareUpgrade();
            }
        }

        Item {
            // Just used as a spacer
            height: 20
            width: 10
        }

        TextArea {
            id: statusTextArea
            width: parent.width
            height: 300
            readOnly: true
            frameVisible: false
            style: TextAreaStyle {
                textColor: qgcPal.windowText
                backgroundColor: qgcPal.windowShade
            }
        }
    }
}
