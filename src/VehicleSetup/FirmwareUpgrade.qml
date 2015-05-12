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
import QtQuick.Dialogs 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

QGCView {
    viewComponent: viewPanelComponent

    property string firmwareWarningMessage

    Component {
        id: viewPanelComponent

        QGCViewPanel {
            id: panel

            FirmwareUpgradeController {
                id:             controller
                upgradeButton:  upgradeButton
                progressBar:    progressBar
                statusLog:      statusTextArea
                firmwareType:   FirmwareUpgradeController.StableFirmware

                onShowMessage: {
                    panel.showMessage(title, message, StandardButton.Ok)
                }
            }

            Component {
                id: firmwareWarningComponent

                QGCViewMessage {
                    message: firmwareWarningMessage

                    function accept() {
                        panel.hideDialog()
                        controller.doFirmwareUpgrade();
                    }
                }
            }

            Column {
                anchors.fill: parent

                QGCLabel {
                    text: "FIRMWARE UPDATE"
                    font.pointSize: ScreenTools.fontPointFactor * (20);
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
                            if (controller.activeQGCConnections()) {
                                panel.showMessage("Firmware Upgrade",
                                                    "There are still vehicles connected to QGroundControl. " +
                                                    "You must disconnect all vehicles from QGroundControl prior to Firmware Upgrade.",
                                                    StandardButton.Ok)
                                return
                            }

                            if (controller.pluggedInBoard()) {
                                panel.showMessage("Firmware Upgrade",
                                                    "You vehicle is currently connected via USB. " +
                                                    "You must unplug your vehicle from USB prior to Firmware Upgrade.",
                                                    StandardButton.Ok)
                                return
                            }

                            controller.firmwareType = firmwareItems.get(firmwareCombo.currentIndex).firmwareType

                            if (controller.firmwareType == 1) {
                                firmwareWarningMessage = "WARNING: BETA FIRMWARE\n" +
                                                            "This firmware version is ONLY intended for beta testers. " +
                                                            "Although it has received FLIGHT TESTING, it represents actively changed code. " +
                                                            "Do NOT use for normal operation.\n\n" +
                                                            "Click Cancel to abort upgrade, Click Ok to Upgrade anwyay"
                                panel.showDialog(firmwareWarningComponent, "Firmware Upgrade", 50, StandardButton.Cancel | StandardButton.Ok)
                            } else if (controller.firmwareType == 2) {
                                firmwareWarningMessage = "WARNING: CONTINUOUS BUILD FIRMWARE\n" +
                                                            "This firmware has NOT BEEN FLIGHT TESTED. " +
                                                            "It is only intended for DEVELOPERS. " +
                                                            "Run bench tests without props first. " +
                                                            "Do NOT fly this without addional safety precautions. " +
                                                            "Follow the mailing list actively when using it.\n\n" +
                                                            "Click Cancel to abort upgrade, Click Ok to Upgrade anwyay"
                                panel.showDialog(firmwareWarningComponent, "Firmware Upgrade", 50, StandardButton.Cancel | StandardButton.Ok)
                            } else {
                                controller.doFirmwareUpgrade();
                            }
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
                    font.pointSize: ScreenTools.defaultFontPointSize
                    
                    text: qsTr("Please disconnect all vehicles from QGroundControl before selecting Upgrade.")

                    style: TextAreaStyle {
                        textColor:          qgcPal.text
                        backgroundColor:    qgcPal.windowShade
                    }
                }
            } // Column
        } // QGCViewPanel
    } // Component - View Panel
} // QGCView