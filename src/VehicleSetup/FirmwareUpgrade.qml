/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.FirmwareUpgradeController 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    width: 600
    height: 600

    property var qgcPal: QGCPalette { colorGroupEnabled: true }
    property ScreenTools screenTools: ScreenTools { }
    property FirmwareUpgradeController controller: FirmwareUpgradeController {
        upgradeButton: upgradeButton
        progressBar: progressBar
        statusLog: statusTextArea
        firmwareType: FirmwareUpgradeController.StableFirmware
    }

    color: qgcPal.window

    Column {
        anchors.fill:parent

        QGCLabel {
            text: "FIRMWARE UPDATE"
            font.pointSize: screenTools.dpiAdjustedPointSize(20);
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

            width:			parent.width
            height:			300
            readOnly:		true
            frameVisible:	false
			font.pointSize: qgcPal.dpiAdjustedDefaultFontPointSize
            
			text: qsTr("Please disconnect all connections and unplug board from USB before selecting Upgrade.")

            style: TextAreaStyle {
                textColor: qgcPal.text
                backgroundColor: qgcPal.windowShade
            }
        }
    }
}
