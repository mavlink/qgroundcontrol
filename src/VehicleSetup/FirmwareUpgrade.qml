/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             firmwarePage
    pageComponent:  firmwarePageComponent
    pageName:       qsTr("Firmware")
    showAdvanced:   activeVehicle && activeVehicle.apmFirmware

    signal cancelDialog

    Component {
        id: firmwarePageComponent

        ColumnLayout {
            width:   availableWidth
            height:  availableHeight
            spacing: ScreenTools.defaultFontPixelHeight

            // Those user visible strings are hard to translate because we can't send the
            // HTML strings to translation as this can create a security risk. we need to find
            // a better way to hightlight them, or use less highlights.

            // User visible strings
            readonly property string title:             qsTr("Firmware Setup") // Popup dialog title
            readonly property string highlightPrefix:   "<font color=\"" + qgcPal.warningText + "\">"
            readonly property string highlightSuffix:   "</font>"
            readonly property string welcomeText:       qsTr("%1 can upgrade the firmware on Pixhawk devices, SiK Radios and PX4 Flow Smart Cameras.").arg(QGroundControl.appName)
            readonly property string welcomeTextSingle: qsTr("Update the autopilot firmware to the latest version")
            readonly property string plugInText:        "<big>" + highlightPrefix + "Plug in your device" + highlightSuffix + " via USB to " + highlightPrefix + "start" + highlightSuffix + " firmware upgrade.</big>"
            readonly property string flashFailText:     "If upgrade failed, make sure to connect " + highlightPrefix + "directly" + highlightSuffix + " to a powered USB port on your computer, not through a USB hub. " +
                                                        "Also make sure you are only powered via USB " + highlightPrefix + "not battery" + highlightSuffix + "."
            readonly property string qgcUnplugText1:    qsTr("All %1 connections to vehicles must be ").arg(QGroundControl.appName) + highlightPrefix + " disconnected " + highlightSuffix + "prior to firmware upgrade."
            readonly property string qgcUnplugText2:    highlightPrefix + "<big>Please unplug your Pixhawk and/or Radio from USB.</big>" + highlightSuffix

            readonly property int _defaultFimwareTypePX4:   12
            readonly property int _defaultFimwareTypeAPM:   3

            property var    _firmwareUpgradeSettings:   QGroundControl.settingsManager.firmwareUpgradeSettings
            property var    _defaultFirmwareFact:       _firmwareUpgradeSettings.defaultFirmwareType
            property bool   _defaultFirmwareIsPX4:      true

            property string firmwareWarningMessage
            property bool   firmwareWarningMessageVisible:  false
            property bool   initialBoardSearch:             true
            property string firmwareName

            property bool _singleFirmwareMode:          QGroundControl.corePlugin.options.firmwareUpgradeSingleURL.length != 0   ///< true: running in special single firmware download mode

            function cancelFlash() {
                statusTextArea.append(highlightPrefix + qsTr("Upgrade cancelled") + highlightSuffix)
                statusTextArea.append("------------------------------------------")
                controller.cancel()
            }

            function setupPageCompleted() {
                controller.startBoardSearch()
                _defaultFirmwareIsPX4 = _defaultFirmwareFact.rawValue === _defaultFimwareTypePX4 // we don't want this to be bound and change as radios are selected
            }

            QGCFileDialog {
                id:                 customFirmwareDialog
                title:              qsTr("Select Firmware File")
                nameFilters:        [qsTr("Firmware Files (*.px4 *.apj *.bin *.ihx)"), qsTr("All Files (*)")]
                selectExisting:     true
                folder:             QGroundControl.settingsManager.appSettings.logSavePath
                onAcceptedForLoad: {
                    controller.flashFirmwareUrl(file)
                    close()
                }
            }

            FirmwareUpgradeController {
                id:             controller
                progressBar:    progressBar
                statusLog:      statusTextArea

                property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                onActiveVehicleChanged: {
                    if (!activeVehicle) {
                        statusTextArea.append(plugInText)
                    }
                }

                onNoBoardFound: {
                    initialBoardSearch = false
                    if (!QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                        statusTextArea.append(plugInText)
                    }
                }

                onBoardGone: {
                    initialBoardSearch = false
                    if (!QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                        statusTextArea.append(plugInText)
                    }
                }

                onBoardFound: {
                    if (initialBoardSearch) {
                        // Board was found right away, so something is already plugged in before we've started upgrade
                        statusTextArea.append(qgcUnplugText1)
                        statusTextArea.append(qgcUnplugText2)

                        var availableDevices = controller.availableBoardsName()
                        if(availableDevices.length > 1) {
                            statusTextArea.append(highlightPrefix + qsTr("Multiple devices detected! Remove all detected devices to perform the firmware upgrade."))
                            statusTextArea.append(qsTr("Detected [%1]: ").arg(availableDevices.length) + availableDevices.join(", "))
                        }
                        if(QGroundControl.multiVehicleManager.activeVehicle) {
                            QGroundControl.multiVehicleManager.activeVehicle.autoDisconnect = true
                        }
                    } else {
                        // We end up here when we detect a board plugged in after we've started upgrade
                        statusTextArea.append(highlightPrefix + qsTr("Found device") + highlightSuffix + ": " + controller.boardType)
                        if (controller.pixhawkBoard || controller.px4FlowBoard) {
                            mainWindow.showComponentDialog(pixhawkFirmwareSelectDialogComponent, title, mainWindow.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                        }
                    }
                }

                onError: {
                    statusTextArea.append(flashFailText)
                    firmwarePage.cancelDialog()
                }
            }

            Component {
                id: pixhawkFirmwareSelectDialogComponent

                QGCViewDialog {
                    id: pixhawkFirmwareSelectDialog

                    property bool showFirmwareTypeSelection:    _advanced.checked
                    property bool px4Flow:                      controller.px4FlowBoard

                    function firmwareVersionChanged(model) {
                        firmwareWarningMessageVisible = false
                        // All of this bizarre, setting model to null and index to 1 and then to 0 is to work around
                        // strangeness in the combo box implementation. This sequence of steps correctly changes the combo model
                        // without generating any warnings and correctly updates the combo text with the new selection.
                        firmwareBuildTypeCombo.model = null
                        firmwareBuildTypeCombo.model = model
                        firmwareBuildTypeCombo.currentIndex = 1
                        firmwareBuildTypeCombo.currentIndex = 0
                    }

                    function updatePX4VersionDisplay() {
                        var versionString = ""
                        if (_advanced.checked) {
                            switch (controller.selectedFirmwareBuildType) {
                            case FirmwareUpgradeController.StableFirmware:
                                versionString = controller.px4StableVersion
                                break
                            case FirmwareUpgradeController.BetaFirmware:
                                versionString = controller.px4BetaVersion
                                break
                            }
                        } else {
                            versionString = controller.px4StableVersion
                        }
                        px4FlightStackRadio.text = qsTr("PX4 Pro ") + versionString
                        //px4FlightStackRadio2.text = qsTr("PX4 Pro ") + versionString
                    }

                    Component.onCompleted: {
                        firmwarePage.advanced = false
                        firmwarePage.showAdvanced = false
                        updatePX4VersionDisplay()
                    }

                    function accept() {
                        if (_singleFirmwareMode) {
                            controller.flashSingleFirmwareMode(controller.selectedFirmwareBuildType)
                        } else {
                            var stack
                            var firmwareBuildType = firmwareBuildTypeCombo.model.get(firmwareBuildTypeCombo.currentIndex).firmwareType
                            var vehicleType = FirmwareUpgradeController.DefaultVehicleFirmware

                            if (px4Flow) {
                                stack = px4FlowTypeSelectionCombo.model.get(px4FlowTypeSelectionCombo.currentIndex).stackType
                                vehicleType = FirmwareUpgradeController.DefaultVehicleFirmware
                            } else {
                                stack = apmFlightStack.checked ? FirmwareUpgradeController.AutoPilotStackAPM : FirmwareUpgradeController.AutoPilotStackPX4
                                if (apmFlightStack.checked) {
                                    if (firmwareBuildType === FirmwareUpgradeController.CustomFirmware) {
                                        vehicleType = apmVehicleTypeCombo.currentIndex
                                    } else {
                                        if (controller.apmFirmwareNames.length === 0) {
                                            // Not ready yet, or no firmware available
                                            return
                                        }
                                        var firmwareUrl = controller.apmFirmwareUrls[ardupilotFirmwareSelectionCombo.currentIndex]
                                        if (firmwareUrl == "") {
                                            return
                                        }
                                        controller.flashFirmwareUrl(controller.apmFirmwareUrls[ardupilotFirmwareSelectionCombo.currentIndex])
                                        hideDialog()
                                        return
                                    }
                                }
                            }
                            //-- If custom, get file path
                            if (firmwareBuildType === FirmwareUpgradeController.CustomFirmware) {
                                customFirmwareDialog.openForLoad()
                            } else {
                                controller.flash(stack, firmwareBuildType, vehicleType)
                            }
                            hideDialog()
                        }
                    }

                    function reject() {
                        hideDialog()
                        cancelFlash()
                    }

                    Connections {
                        target:         firmwarePage
                        onCancelDialog: reject()
                    }

                    ListModel {
                        id: firmwareBuildTypeList

                        ListElement {
                            text:           qsTr("Standard Version (stable)")
                            firmwareType:   FirmwareUpgradeController.StableFirmware
                        }
                        ListElement {
                            text:           qsTr("Beta Testing (beta)")
                            firmwareType:   FirmwareUpgradeController.BetaFirmware
                        }
                        ListElement {
                            text:           qsTr("Developer Build (master)")
                            firmwareType:   FirmwareUpgradeController.DeveloperFirmware
                        }
                        ListElement {
                            text:           qsTr("Custom firmware file...")
                            firmwareType:   FirmwareUpgradeController.CustomFirmware
                        }
                    }

                    ListModel {
                        id: px4FlowFirmwareList

                        ListElement {
                            text:           qsTr("PX4 Pro")
                            stackType:   FirmwareUpgradeController.PX4FlowPX4
                        }
                        ListElement {
                            text:           qsTr("ArduPilot")
                            stackType:   FirmwareUpgradeController.PX4FlowAPM
                        }
                    }

                    ListModel {
                        id: px4FlowTypeList

                        ListElement {
                            text:           qsTr("Standard Version (stable)")
                            firmwareType:   FirmwareUpgradeController.StableFirmware
                        }
                        ListElement {
                            text:           qsTr("Custom firmware file...")
                            firmwareType:   FirmwareUpgradeController.CustomFirmware
                        }
                    }

                    ListModel {
                        id: singleFirmwareModeTypeList

                        ListElement {
                            text:           qsTr("Standard Version")
                            firmwareType:   FirmwareUpgradeController.StableFirmware
                        }
                        ListElement {
                            text:           qsTr("Custom firmware file...")
                            firmwareType:   FirmwareUpgradeController.CustomFirmware
                        }
                    }

                    QGCFlickable {
                        anchors.fill:   parent
                        contentHeight:  mainColumn.height

                        Column {
                            id:             mainColumn
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            spacing:        defaultTextHeight

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text:       (_singleFirmwareMode || !QGroundControl.apmFirmwareSupported) ? _singleFirmwareLabel : (px4Flow ? _px4FlowLabel : _pixhawkLabel)

                                readonly property string _px4FlowLabel:          qsTr("Detected PX4 Flow board. The firmware you use on the PX4 Flow must match the AutoPilot firmware type you are using on the vehicle:")
                                readonly property string _pixhawkLabel:          qsTr("Detected Pixhawk board. You can select from the following flight stacks:")
                                readonly property string _singleFirmwareLabel:   qsTr("Press Ok to upgrade your vehicle.")
                            }

                            QGCLabel { text: qsTr("Flight Stack"); visible: QGroundControl.apmFirmwareSupported }

                            Column {

                                Component.onCompleted: {
                                    if(!QGroundControl.apmFirmwareSupported) {
                                        _defaultFirmwareFact.rawValue = _defaultFimwareTypePX4
                                        firmwareVersionChanged(firmwareBuildTypeList)
                                    }
                                }

                                QGCRadioButton {
                                    id:             px4FlightStackRadio
                                    text:           qsTr("PX4 Pro ")
                                    textBold:       _defaultFirmwareIsPX4
                                    checked:        _defaultFirmwareIsPX4
                                    visible:        !_singleFirmwareMode && !px4Flow && QGroundControl.apmFirmwareSupported

                                    onClicked: {
                                        _defaultFirmwareFact.rawValue = _defaultFimwareTypePX4
                                        firmwareVersionChanged(firmwareBuildTypeList)
                                    }
                                }

                                QGCRadioButton {
                                    id:             apmFlightStack
                                    text:           qsTr("ArduPilot")
                                    textBold:       !_defaultFirmwareIsPX4
                                    checked:        !_defaultFirmwareIsPX4
                                    visible:        !_singleFirmwareMode && !px4Flow && QGroundControl.apmFirmwareSupported

                                    onClicked: {
                                        _defaultFirmwareFact.rawValue = _defaultFimwareTypeAPM
                                        firmwareVersionChanged(firmwareBuildTypeList)
                                    }
                                }
                            }

                            FactComboBox {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                visible:        !px4Flow && apmFlightStack.checked
                                fact:           _firmwareUpgradeSettings.apmChibiOS
                                indexModel:     false
                            }

                            FactComboBox {
                                id:             apmVehicleTypeCombo
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                visible:        !px4Flow && apmFlightStack.checked
                                fact:           _firmwareUpgradeSettings.apmVehicleType
                                indexModel:     false
                            }

                            QGCComboBox {
                                id:             ardupilotFirmwareSelectionCombo
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                visible:        !px4Flow && apmFlightStack.checked && !controller.downloadingFirmwareList && controller.apmFirmwareNames.length !== 0
                                model:          controller.apmFirmwareNames

                                onModelChanged: console.log("model", model)
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("Downloading list of available firmwares...")
                                visible:        controller.downloadingFirmwareList
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("No Firmware Available")
                                visible:        !controller.downloadingFirmwareList && (QGroundControl.apmFirmwareSupported && controller.apmFirmwareNames.length === 0)
                            }

                            QGCComboBox {
                                id:             px4FlowTypeSelectionCombo
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                visible:        px4Flow
                                model:          px4FlowFirmwareList
                                textRole:       "text"
                                currentIndex:   _defaultFirmwareIsPX4 ? 0 : 1
                            }

                            Row {
                                width:      parent.width
                                spacing:    ScreenTools.defaultFontPixelWidth / 2
                                visible:    !px4Flow

                                Rectangle {
                                    height:     1
                                    width:      ScreenTools.defaultFontPixelWidth * 5
                                    color:      qgcPal.text
                                    anchors.verticalCenter: _advanced.verticalCenter
                                }

                                QGCCheckBox {
                                    id:         _advanced
                                    text:       qsTr("Advanced settings")
                                    checked:    px4Flow ? true : false

                                    onClicked: {
                                        firmwareBuildTypeCombo.currentIndex = 0
                                        firmwareWarningMessageVisible = false
                                        updatePX4VersionDisplay()
                                    }
                                }

                                Rectangle {
                                    height:     1
                                    width:      ScreenTools.defaultFontPixelWidth * 5
                                    color:      qgcPal.text
                                    anchors.verticalCenter: _advanced.verticalCenter
                                }
                            }

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                visible:    showFirmwareTypeSelection
                                text:       _singleFirmwareMode ?  qsTr("Select the standard version or one from the file system (previously downloaded):") :
                                                                  (px4Flow ? qsTr("Select which version of the firmware you would like to install:") :
                                                                             qsTr("Select which version of the above flight stack you would like to install:"))
                            }

                            QGCComboBox {
                                id:             firmwareBuildTypeCombo
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                visible:        showFirmwareTypeSelection
                                textRole:       "text"
                                model:          _singleFirmwareMode ? singleFirmwareModeTypeList : (px4Flow ? px4FlowTypeList : firmwareBuildTypeList)
                                currentIndex:   controller.selectedFirmwareBuildType

                                onActivated: {
                                    controller.selectedFirmwareBuildType = model.get(index).firmwareType
                                    if (model.get(index).firmwareType === FirmwareUpgradeController.BetaFirmware) {
                                        firmwareWarningMessageVisible = true
                                        firmwareVersionWarningLabel.text = qsTr("WARNING: BETA FIRMWARE. ") +
                                                qsTr("This firmware version is ONLY intended for beta testers. ") +
                                                qsTr("Although it has received FLIGHT TESTING, it represents actively changed code. ") +
                                                qsTr("Do NOT use for normal operation.")
                                    } else if (model.get(index).firmwareType === FirmwareUpgradeController.DeveloperFirmware) {
                                        firmwareWarningMessageVisible = true
                                        firmwareVersionWarningLabel.text = qsTr("WARNING: CONTINUOUS BUILD FIRMWARE. ") +
                                                qsTr("This firmware has NOT BEEN FLIGHT TESTED. ") +
                                                qsTr("It is only intended for DEVELOPERS. ") +
                                                qsTr("Run bench tests without props first. ") +
                                                qsTr("Do NOT fly this without additional safety precautions. ") +
                                                qsTr("Follow the mailing list actively when using it.")
                                    } else {
                                        firmwareWarningMessageVisible = false
                                    }
                                    updatePX4VersionDisplay()
                                }
                            }

                            QGCLabel {
                                id:         firmwareVersionWarningLabel
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                visible:    firmwareWarningMessageVisible
                            }
                        } // Column
                    } // QGCFLickable
                } // QGCViewDialog
            } // Component - pixhawkFirmwareSelectDialogComponent

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

            ProgressBar {
                id:                     progressBar
                Layout.preferredWidth:  parent.width
                visible:                !flashBootloaderButton.visible
            }

            QGCButton {
                id:         flashBootloaderButton
                text:       qsTr("Flash ChibiOS Bootloader")
                visible:    firmwarePage.advanced
                onClicked:  activeVehicle.flashBootloader()
            }

            TextArea {
                id:                 statusTextArea
                Layout.preferredWidth:              parent.width
                Layout.fillHeight:  true
                readOnly:           true
                frameVisible:       false
                font.pointSize:     ScreenTools.defaultFontPointSize
                textFormat:         TextEdit.RichText
                text:               _singleFirmwareMode ? welcomeTextSingle : welcomeText

                style: TextAreaStyle {
                    textColor:          qgcPal.text
                    backgroundColor:    qgcPal.windowShade
                }
            }
        } // ColumnLayout
    } // Component
} // SetupPage
