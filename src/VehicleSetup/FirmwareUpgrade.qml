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

    // User visible strings
    readonly property string title:             "FIRMWARE UPDATE"
    readonly property string highlightPrefix:   "<font color=\"yellow\">"
    readonly property string highlightSuffix:   "</font>"
    readonly property string welcomeText:       "QGroundControl can upgrade the firmware on Pixhawk devices, 3DR Radios and PX4 Flow Smart Cameras."
    readonly property string plugInText:        highlightPrefix + "Plug in your device" + highlightSuffix + " via USB to " + highlightPrefix + "start" + highlightSuffix + " firmware upgrade"
    readonly property string qgcDisconnectText: "All QGroundControl connections to vehicles must be disconnected prior to firmware upgrade. " +
                                                    "Click " + highlightPrefix + "Disconnect" + highlightSuffix + " in the toolbar above."
    property string usbUnplugText:              "Device must be disconnected from USB to start firmware upgrade. " +
                                                    highlightPrefix + "Disconnect {0}" + highlightSuffix + " from usb."

    property string firmwareWarningMessage
    property bool   controllerCompleted:      false
    property bool   initialBoardSearch:       true
    property string firmwareName

    function cancelFlash() {
        statusTextArea.append(highlightPrefix + "Upgrade cancelled" + highlightSuffix)
        statusTextArea.append("------------------------------------------")
        controller.cancel()
        flashCompleteWaitTimer.running = true
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    FirmwareUpgradeController {
        id:             controller
        progressBar:    progressBar
        statusLog:      statusTextArea

        Component.onCompleted: {
            controllerCompleted = true
            if (qgcView.completedSignalled) {
                // We can only start the board search when the Qml and Controller are completely done loading
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
                // Board was found right away, so something is already plugged in before we've started upgrade
                if (controller.qgcConnections) {
                    statusTextArea.append(qgcDisconnectText)
                } else {
                    statusTextArea.append(usbUnplugText.replace('{0}', controller.boardType))
                }
            } else {
                // We end up here when we detect a board plugged in after we've started upgrade
                statusTextArea.append(highlightPrefix + "Found device" + highlightSuffix + ": " + controller.boardType)
                if (controller.boardType == "Pixhawk" || controller.boardType == "AeroCore") {
                    showDialog(pixhawkFirmwareSelectDialog, title, 50, StandardButton.Ok | StandardButton.Cancel)
                 }
             }
         }

        onError: {
            hideDialog()
            flashCompleteWaitTimer.running = true
        }

        onFlashComplete: flashCompleteWaitTimer.running = true
    }

    onCompleted: {
        if (controllerCompleted) {
            // We can only start the board search when the Qml and Controller are completely done loading
            controller.startBoardSearch()
        }
    }

    // After a flash completes we start this timer to trigger resetting the ui back to it's initial state of being ready to
    // flash another board. We do this only after the timer triggers to leave the results of the previous flash on the screen
    // for a small amount amount of time.

    Timer {
        id:         flashCompleteWaitTimer
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

                function firmwareVersionChanged(model) {
                    firmwareVersionWarningLabel.visible = false
                    // All of this bizarre, setting model to null and index to 1 and then to 0 is to work around
                    // strangeness in the combo box implementation. This sequence of steps correctly changes the combo model
                    // without generating any warnings and correctly updates the combo text with the new selection.
                    firmwareVersionCombo.model = null
                    firmwareVersionCombo.model = model
                    firmwareVersionCombo.currentIndex = 1
                    firmwareVersionCombo.currentIndex = 0
                }

                QGCRadioButton {
                    id:             px4FlightStack
                    checked:        true
                    exclusiveGroup: firmwareGroup
                    text:           "PX4 Flight Stack (full QGC support)"

                    onClicked: parent.firmwareVersionChanged(px4FirmwareTypeList)
                }

                QGCRadioButton {
                    id:             apmFlightStack
                    exclusiveGroup: firmwareGroup
                    text:           "APM Flight Stack (partial QGC support)"

                    onClicked: parent.firmwareVersionChanged(apmFirmwareTypeList)
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
                    model:      px4FirmwareTypeList

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

            QGCButton {
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                anchors.left:       advancedMode.right
                anchors.bottom:     parent.bottom
                text:               "Help me pick a flight stack"
                onClicked:          Qt.openUrlExternally("http://pixhawk.org/choice")
            }
        } // QGCViewDialog
    } // Component - pixhawkFirmwareSelectDialog


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

        QGCLabel {
            id:             titleLabel
            text:           title
            font.pixelSize: ScreenTools.largeFontPixelSize
        }

        ProgressBar {
            id:                 progressBar
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        titleLabel.bottom
            width:              parent.width
        }

        TextArea {
            id:                 statusTextArea
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        progressBar.bottom
            anchors.bottom:     parent.bottom
            width:              parent.width
            readOnly:           true
            frameVisible:       false
            font.pixelSize:     ScreenTools.defaultFontPixelSize
            textFormat:         TextEdit.RichText
            text:               welcomeText

            style: TextAreaStyle {
                textColor:          qgcPal.text
                backgroundColor:    qgcPal.windowShade
            }
        }
    } // QGCViewPabel
} // QGCView
