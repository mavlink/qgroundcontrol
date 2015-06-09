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
    id:         qgcView
    viewPanel:  panel

    // User visible string
    readonly property string title:             "FIRMWARE UPDATE"
    readonly property string welcomeText:       "QGroundControl can upgrade the firmware on Pixhawk devices, 3DR Radios and PX4 Flow Smart Cameras."
    readonly property string plugInText:        "<font color=\"yellow\">Plug in your device</font> via USB to <font color=\"yellow\">start</font> firmware upgrade"
    readonly property string qgcDisconnectText: "All QGroundControl connections to vehicles must be disconnected prior to firmware upgrade.\n" +
                                                    "Click <font color=\"yellow\">Disconnect</font> in the toolbar above."
    property string usbUnplugText:              "Device must be disconnected from USB to start firmware upgrade.\n" +
                                                    "<font color=\"yellow\">Disconnect {0}</font> from usb."

    property string firmwareWarningMessage
    property bool   controllerCompleted:      false
    property bool   controllerAndViewReady:   false
    property bool   initialBoardSearch:       true
    property string firmwareName

    function cancelFlash() {
        statusTextArea.append("Upgrade cancelled")
        controller.cancel()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    FirmwareUpgradeController {
        id:             controller
        progressBar:    progressBar
        statusLog:      statusTextArea

        Component.onCompleted: {
            controllerCompleted = true
            if (qgcView.completedSignalled) {
                controllerAndViewReady = true
                controller.startBoardSearch()
            }
        }

        onNoBoardFound: {
            initialBoardSearch = false
            statusTextArea.append(plugInText)
        }

        onBoardGone: {
            initialBoardSearch = false
            statusTextArea.append(plugInText)
        }

        onBoardFound: {
            if (initialBoardSearch) {
                if (controller.qgcConnections) {
                    statusTextArea.append(qgcDisconnectText)
                } else {
                    statusTextArea.append(usbUnplugText.replace('{0}', controller.boardType))
                }
            } else {
                statusTextArea.append("Found device: " + controller.boardType)
                if (controller.boardType == "Pixhawk") {
                    showDialog(pixhawkFirmwareSelectDialog, title, 50, StandardButton.Ok | StandardButton.Cancel)
                }
            }
        }

        onError: hideDialog()

        onFlashComplete: flashCompleteWait.running = true
    }

    onCompleted: {
        if (controllerCompleted) {
            controllerAndViewReady = true
            controller.start()
        }
    }

    Timer {
        id:         flashCompleteWait
        interval:   15000

        onTriggered: {
            initialBoardSearch = true
            progressBar.value = 0
            statusTextArea.append(welcomeText)
            controller.startBoardSearch()
        }
    }

    Component {
        id: pixhawkFirmwareSelectDialog

        QGCViewDialog {
            anchors.fill: parent

            property bool showVersionSelection: apmFlightStack.checked || advancedMode.checked

            function accept() {
                hideDialog()
                controller.flash(firmwareVersionCombo.model.get(firmwareVersionCombo.currentIndex).firmwareType)
            }

            function reject() {
                cancelFlash()
                hideDialog()
            }

            ExclusiveGroup {
                id: firmwareGroup
            }

            ListModel {
                id: px4FirmwareTypeList

                ListElement {
                    text: qsTr("Standard Version (stable)");
                    firmwareType: FirmwareUpgradeController.PX4StableFirmware
                }
                ListElement {
                    text: qsTr("Beta Testing (beta)");
                    firmwareType: FirmwareUpgradeController.PX4BetaFirmware
                }
                ListElement {
                    text: qsTr("Developer Build (master)");
                    firmwareType: FirmwareUpgradeController.PX4DeveloperFirmware
                }
                ListElement {
                    text: qsTr("Custom firmware file...");
                    firmwareType: FirmwareUpgradeController.PX4CustomFirmware
                }
            }

            ListModel {
                id: apmFirmwareTypeList

                ListElement {
                    text: "ArduCopter Quad"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterQuadFirmware
                }
                ListElement {
                    text: "ArduCopter X8"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterX8Firmware
                }
                ListElement {
                    text: "ArduCopter Hexa"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterHexaFirmware
                }
                ListElement {
                    text: "ArduCopter Octo"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterOctoFirmware
                }
                ListElement {
                    text: "ArduCopter Y"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterYFirmware
                }
                ListElement {
                    text: "ArduCopter Y6"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterY6Firmware
                }
                ListElement {
                    text: "ArduCopter Heli"
                    firmwareType: FirmwareUpgradeController.ApmArduCopterHeliFirmware
                }
                ListElement {
                    text: "ArduPlane"
                    firmwareType: FirmwareUpgradeController.ApmArduPlaneFirmware
                }
                ListElement {
                    text: "Rover"
                    firmwareType: FirmwareUpgradeController.ApmRoverFirmware
                }
            }

            Column {
                anchors.fill:   parent
                spacing:        defaultTextHeight

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       "Detected Pixhawk board. You can select from the following flight stacks:"
                }

                QGCRadioButton {
                    id:             px4FlightStack
                    checked:        true
                    exclusiveGroup: firmwareGroup
                    text:           "PX4 Flight Stack (full QGC support)"

                    onClicked: firmwareVersionWarningLabel.visible = false
                }

                QGCRadioButton {
                    id:             apmFlightStack
                    exclusiveGroup: firmwareGroup
                    text:           "APM Flight Stack (partial QGC support)"

                    onClicked: firmwareVersionWarningLabel.visible = false
                }

                QGCButton {
                    text:       "Help me pick a flight stack"
                    onClicked:  Qt.openUrlExternally("http://pixhawk.org/choice")
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    visible:    showVersionSelection
                    text:       "Select which version of the above flight stack you would like to install:"
                }

                QGCComboBox {
                    id:         firmwareVersionCombo
                    width:      200
                    visible:    showVersionSelection
                    model:      px4FlightStack.checked ? px4FirmwareTypeList : apmFirmwareTypeList

                    onActivated: {
                        if (model.get(index).firmwareType == FirmwareUpgradeController.PX4BetaFirmware) {
                            firmwareVersionWarningLabel.visible = true
                            firmwareVersionWarningLabel.text = "WARNING: BETA FIRMWARE. " +
                                                                    "This firmware version is ONLY intended for beta testers. " +
                                                                    "Although it has received FLIGHT TESTING, it represents actively changed code. " +
                                                                    "Do NOT use for normal operation."
                        } else if (model.get(index).firmwareType == FirmwareUpgradeController.PX4DeveloperFirmware) {
                            firmwareVersionWarningLabel.visible = true
                            firmwareVersionWarningLabel.text = "WARNING: CONTINUOUS BUILD FIRMWARE. " +
                                                                    "This firmware has NOT BEEN FLIGHT TESTED. " +
                                                                    "It is only intended for DEVELOPERS. " +
                                                                    "Run bench tests without props first. " +
                                                                    "Do NOT fly this without addional safety precautions. " +
                                                                    "Follow the mailing list actively when using it."
                        } else {
                            firmwareVersionWarningLabel.visible = false
                        }
                    }
                }

                QGCLabel {
                    id:         firmwareVersionWarningLabel
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    visible:    false
                }


            }

            QGCCheckBox {
                id:             advancedMode
                anchors.bottom: parent.bottom
                text:           "Advanced mode"

                onClicked: {
                    firmwareVersionCombo.currentIndex = 0
                    firmwareVersionWarningLabel.visible = false
                }
            }
        }
    }

/*
    Component {
        id: tripleFirmwareVersionSelectDialog

        QGCViewDialog {
            anchors.fill: parent

            function accept() {
                controller.firmwareType = firmwareItems.get(firmwareCombo.currentIndex).firmwareType

                if (controller.firmwareType == 1) {
                    firmwareWarningMessage = "WARNING: BETA FIRMWARE\n" +
                                                "This firmware version is ONLY intended for beta testers. " +
                                                "Although it has received FLIGHT TESTING, it represents actively changed code. " +
                                                "Do NOT use for normal operation.\n\n" +
                                                "Click Cancel to abort upgrade, Click Ok to Upgrade anwyay"
                    showDialog(firmwareWarningDialog, title, 50, StandardButton.Cancel | StandardButton.Ok)
                } else if (controller.firmwareType == 2) {
                    firmwareWarningMessage = "WARNING: CONTINUOUS BUILD FIRMWARE\n" +
                                                "This firmware has NOT BEEN FLIGHT TESTED. " +
                                                "It is only intended for DEVELOPERS. " +
                                                "Run bench tests without props first. " +
                                                "Do NOT fly this without addional safety precautions. " +
                                                "Follow the mailing list actively when using it.\n\n" +
                                                "Click Cancel to abort upgrade, Click Ok to Upgrade anwyay"
                    showDialog(firmwareWarningDialog, title, 50, StandardButton.Cancel | StandardButton.Ok)
                } else {
                    controller.doFirmwareUpgrade();
                }
            }

            function reject() {
                cancelFlash()
                hideDialog()
            }

            Column {
                anchors.fill:   parent
                spacing:        defaultTextHeight

                QGCLabel {
                    text: firmwareName
                }

                Row {
                    spacing:    10

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
                        model: firmwareItems
                    }
                }
            }
        } // QGCViewDialog
    } // Component
*/

    Component {
        id: firmwareWarningDialog

        QGCViewMessage {
            message: firmwareWarningMessage

            function accept() {
                hideDialog()
                controller.doFirmwareUpgrade();
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Column {
            anchors.fill:   parent
            spacing:        10

            QGCLabel {
                text:           title
                font.pointSize: ScreenTools.largeFontPointSize
            }

            ProgressBar {
                id: progressBar
                width: parent.width
            }

            TextArea {
                id: statusTextArea

                width:			parent.width
                height:         parent.height - x
                readOnly:		true
                frameVisible:	false
                font.pointSize: ScreenTools.defaultFontPointSize
                textFormat:     TextEdit.RichText
                text:           welcomeText

                style: TextAreaStyle {
                    textColor:          qgcPal.text
                    backgroundColor:    qgcPal.windowShade
                }
            }
        } // Column
    }
} // QGCView