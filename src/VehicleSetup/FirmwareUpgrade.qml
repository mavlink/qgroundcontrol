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

/*
               _ui->statusLog->setText(tr("WARNING: BETA FIRMWARE\n"
-                                           "This firmware version is ONLY intended for beta testers. "
-                                           "Although it has received FLIGHT TESTING, it represents actively changed code. Do NOT use for normal operation.\n\n"
-                                           SELECT_FIRMWARE_LICENSE));
-                break;
-                
-            case 2:
-                _ui->statusLog->setText(tr("WARNING: CONTINUOUS BUILD FIRMWARE\n"
-                                           "This firmware has NOT BEEN FLIGHT TESTED. "
-                                           "It is only intended for DEVELOPERS. Run bench tests without props first. "
-                                           "Do NOT fly this without addional safety precautions. Follow the mailing "
-                                           "list actively when using it.\n\n"
-                                           SELECT_FIRMWARE_LICENSE));
*/
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
