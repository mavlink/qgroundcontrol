import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id:             firmwarePage
    pageComponent:  firmwarePageComponent
    pageName:       qsTr("Firmware")
    showAdvanced:   globals.activeVehicle && globals.activeVehicle.apmFirmware

    Component {
        id: firmwarePageComponent

        ColumnLayout {
            width:   availableWidth
            height:  availableHeight
            spacing: ScreenTools.defaultFontPixelHeight

            // Those user visible strings are hard to translate because we can't send the
            // HTML strings to translation as this can create a security risk. we need to find
            // a better way to highlight them, or use less highlights.

            // User visible strings
            readonly property string title:             qsTr("Firmware Setup") // Popup dialog title
            readonly property string highlightPrefix:   "<font color=\"" + qgcPal.warningText + "\">"
            readonly property string highlightSuffix:   "</font>"
            readonly property string welcomeText:       qsTr("%1 can upgrade the firmware on Pixhawk devices and SiK Radios.").arg(QGroundControl.appName)
            readonly property string welcomeTextSingle: qsTr("Update the autopilot firmware to the latest version")
            readonly property string plugInText:        highlightPrefix + qsTr("Plug in your device") + highlightSuffix + qsTr(" via USB, then select it below and press ") + highlightPrefix + qsTr("Flash") + highlightSuffix + "."
            readonly property string unplugReplugText:  highlightPrefix + qsTr("Now unplug your device and plug it back in to enter bootloader mode.") + highlightSuffix
            readonly property string flashFailText:     qsTr("If upgrade failed, make sure to connect ") + highlightPrefix + qsTr("directly") + highlightSuffix + qsTr(" to a powered USB port on your computer, not through a USB hub. ") +
                                                        qsTr("Also make sure you are only powered via USB ") + highlightPrefix + qsTr("not battery") + highlightSuffix + "."

            readonly property int _defaultFimwareTypePX4:   12
            readonly property int _defaultFimwareTypeAPM:   3

            readonly property int _boardTypePixhawk:    0
            readonly property int _boardTypeSiKRadio:   1

            property var    _firmwareUpgradeSettings:   QGroundControl.settingsManager.firmwareUpgradeSettings
            property var    _defaultFirmwareFact:       _firmwareUpgradeSettings.defaultFirmwareType
            property bool   _defaultFirmwareIsPX4:      true

            property string firmwareWarningMessage
            property bool   firmwareWarningMessageVisible:  false
            property string firmwareName
            property bool   _flashStarted:              false  ///< true: user has clicked Flash, suppress further preselection
            property string _selectedSystemLocation
            property string _selectedDisplayName                ///< snapshot of chosen port's label, used while flashing

            property bool _singleFirmwareMode:          QGroundControl.corePlugin.options.firmwareUpgradeSingleURL.length != 0   ///< true: running in special single firmware download mode

            function setupPageCompleted() {
                controller.startBoardSearch()
                _defaultFirmwareIsPX4 = _defaultFirmwareFact.rawValue === _defaultFimwareTypePX4 // we don't want this to be bound and change as radios are selected
            }

            function _preselectIndex() {
                var ports = controller.availablePorts
                if (ports.length === 0) {
                    return -1
                }
                // Prefer a recognized Pixhawk
                for (var i = 0; i < ports.length; i++) {
                    if (ports[i].boardType === _boardTypePixhawk) {
                        return i
                    }
                }
                // Else prefer a recognized SiK radio
                for (var j = 0; j < ports.length; j++) {
                    if (ports[j].boardType === _boardTypeSiKRadio) {
                        return j
                    }
                }
                // Else first item
                return 0
            }

            function _refreshSelection() {
                if (_flashStarted) {
                    return
                }
                var ports = controller.availablePorts
                if (ports.length === 0) {
                    portCombo.currentIndex = -1
                    _selectedSystemLocation = ""
                    return
                }
                // Try to keep the current selection if its port is still present
                if (_selectedSystemLocation !== "") {
                    for (var i = 0; i < ports.length; i++) {
                        if (ports[i].systemLocation === _selectedSystemLocation) {
                            portCombo.currentIndex = i
                            return
                        }
                    }
                }
                portCombo.currentIndex = _preselectIndex()
                if (portCombo.currentIndex >= 0) {
                    _selectedSystemLocation = ports[portCombo.currentIndex].systemLocation
                }
            }


            FirmwareUpgradeController {
                id:             controller
                progressBar:    progressBar
                statusLog:      statusTextArea

                property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                onActiveVehicleChanged: {
                    if (!globals.activeVehicle && !_flashStarted) {
                        statusTextArea.append(plugInText)
                    }
                }

                onAvailablePortsChanged: _refreshSelection()

                onBoardGone: {
                    if (_flashStarted) {
                        statusTextArea.append(highlightPrefix + qsTr("Device disconnected — waiting for it to reappear in bootloader mode...") + highlightSuffix)
                    }
                }

                onBoardFound: {
                    if (_flashStarted) {
                        statusTextArea.append(highlightPrefix + qsTr("Found device") + highlightSuffix + ": " + controller.boardType)
                        if (QGroundControl.multiVehicleManager.activeVehicle) {
                            QGroundControl.multiVehicleManager.activeVehicle.vehicleLinkManager.autoDisconnect = true
                        }
                    }
                }

                onShowFirmwareSelectDlg:    firmwareSelectDialogFactory.open()
                onError: {
                    statusTextArea.append(flashFailText)
                    _flashStarted = false
                }
                onFlashComplete: _flashStarted = false
            }

            QGCPopupDialogFactory {
                id: firmwareSelectDialogFactory

                dialogComponent: firmwareSelectDialogComponent
            }

            Component {
                id: firmwareSelectDialogComponent

                QGCPopupDialog {
                    id:         firmwareSelectDialog
                    title:      qsTr("Firmware Setup")
                    buttons:    Dialog.Ok | Dialog.Cancel

                    property bool showFirmwareTypeSelection:    _advanced.checked

                    QGCFileDialog {
                        id:                 customFirmwareDialog
                        title:              qsTr("Select Firmware File")
                        nameFilters:        [qsTr("Firmware Files (*.px4 *.apj *.bin *.ihx)"), qsTr("All Files (*)")]
                        folder:             QGroundControl.settingsManager.appSettings.logSavePath
                        onAcceptedForLoad: (file) => {
                            controller.flashFirmwareUrl(file)
                            close()
                            firmwareSelectDialog.close()
                        }
                    }

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

                    Connections {
                        target:     controller
                        onError:    reject()
                    }

                    onAccepted: {
                        if (_singleFirmwareMode) {
                            controller.flashSingleFirmwareMode(controller.selectedFirmwareBuildType)
                        } else {
                            var firmwareBuildType = firmwareBuildTypeCombo.model.get(firmwareBuildTypeCombo.currentIndex).firmwareType
                            var vehicleType = FirmwareUpgradeController.DefaultVehicleFirmware

                            var stack = apmFlightStack.checked ? FirmwareUpgradeController.AutoPilotStackAPM : FirmwareUpgradeController.AutoPilotStackPX4
                            if (apmFlightStack.checked) {
                                if (firmwareBuildType === FirmwareUpgradeController.CustomFirmware) {
                                    vehicleType = apmVehicleTypeCombo.currentIndex
                                } else {
                                    if (controller.apmFirmwareNames.length === 0) {
                                        // Not ready yet, or no firmware available
                                        QGroundControl.showMessageDialog(firmwarePage, firmwareSelectDialog.title, qsTr("Either firmware list is still downloading, or no firmware is available for current selection."))
                                        firmwareSelectDialog.preventClose = true
                                        return
                                    }
                                    if (ardupilotFirmwareSelectionCombo.currentIndex == -1) {
                                        QGroundControl.showMessageDialog(firmwarePage, firmwareSelectDialog.title, qsTr("You must choose a board type."))
                                        firmwareSelectDialog.preventClose = true
                                        return
                                    }

                                    var firmwareUrl = controller.apmFirmwareUrls[ardupilotFirmwareSelectionCombo.currentIndex]
                                    if (firmwareUrl == "") {
                                        QGroundControl.showMessageDialog(firmwarePage, firmwareSelectDialog.title, qsTr("No firmware was found for the current selection."))
                                        firmwareSelectDialog.preventClose = true
                                        return
                                    }
                                    controller.flashFirmwareUrl(controller.apmFirmwareUrls[ardupilotFirmwareSelectionCombo.currentIndex])
                                    return
                                }
                            }
                            //-- If custom, get file path
                            if (firmwareBuildType === FirmwareUpgradeController.CustomFirmware) {
                                customFirmwareDialog.openForLoad()
                            } else {
                                controller.flash(stack, firmwareBuildType, vehicleType)
                            }
                        }
                    }

                    function reject() {
                        statusTextArea.append(highlightPrefix + qsTr("Upgrade cancelled") + highlightSuffix)
                        controller.cancel()
                        close()
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

                    ColumnLayout {
                        width:      Math.max(ScreenTools.defaultFontPixelWidth * 40, firmwareRadiosColumn.width)
                        spacing:    globals.defaultTextHeight / 2

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            text:               (_singleFirmwareMode || !QGroundControl.apmFirmwareSupported) ? _singleFirmwareLabel : _pixhawkLabel

                            readonly property string _pixhawkLabel:          qsTr("Detected Pixhawk board. You can select from the following flight stacks:")
                            readonly property string _singleFirmwareLabel:   qsTr("Press Ok to upgrade your vehicle.")
                        }

                        Column {
                            id:         firmwareRadiosColumn
                            spacing:    0

                            visible: !_singleFirmwareMode && QGroundControl.apmFirmwareSupported

                            Component.onCompleted: {
                                if(!QGroundControl.apmFirmwareSupported) {
                                    _defaultFirmwareFact.rawValue = _defaultFimwareTypePX4
                                    firmwareVersionChanged(firmwareBuildTypeList)
                                }
                            }

                            QGCRadioButton {
                                id:             px4FlightStackRadio
                                text:           qsTr("PX4 Pro ")
                                font.bold:      _defaultFirmwareIsPX4
                                checked:        _defaultFirmwareIsPX4

                                onClicked: {
                                    _defaultFirmwareFact.rawValue = _defaultFimwareTypePX4
                                    firmwareVersionChanged(firmwareBuildTypeList)
                                }
                            }

                            QGCRadioButton {
                                id:             apmFlightStack
                                text:           qsTr("ArduPilot")
                                font.bold:      !_defaultFirmwareIsPX4
                                checked:        !_defaultFirmwareIsPX4

                                onClicked: {
                                    _defaultFirmwareFact.rawValue = _defaultFimwareTypeAPM
                                    firmwareVersionChanged(firmwareBuildTypeList)
                                }
                            }
                        }

                        FactComboBox {
                            Layout.fillWidth:   true
                            visible:            apmFlightStack.checked
                            fact:               _firmwareUpgradeSettings.apmChibiOS
                            indexModel:         false
                        }

                        FactComboBox {
                            id:                 apmVehicleTypeCombo
                            Layout.fillWidth:   true
                            visible:            apmFlightStack.checked
                            fact:               _firmwareUpgradeSettings.apmVehicleType
                            indexModel:         false
                        }

                        QGCComboBox {
                            id:                 ardupilotFirmwareSelectionCombo
                            Layout.fillWidth:   true
                            visible:            apmFlightStack.checked && !controller.downloadingFirmwareList && controller.apmFirmwareNames.length !== 0
                            model:              controller.apmFirmwareNames
                            onModelChanged:     currentIndex = controller.apmFirmwareNamesBestIndex
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            text:               qsTr("Downloading list of available firmwares...")
                            visible:            controller.downloadingFirmwareList
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            text:               qsTr("No Firmware Available")
                            visible:            !controller.downloadingFirmwareList && (QGroundControl.apmFirmwareSupported && controller.apmFirmwareNames.length === 0)
                        }

                        QGCCheckBox {
                            id:         _advanced
                            text:       qsTr("Advanced settings")
                            checked:    false

                            onClicked: {
                                firmwareBuildTypeCombo.currentIndex = 0
                                firmwareWarningMessageVisible = false
                                updatePX4VersionDisplay()
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            visible:            showFirmwareTypeSelection
                            text:               _singleFirmwareMode ?  qsTr("Select the standard version or one from the file system (previously downloaded):") :
                                                                      qsTr("Select which version of the above flight stack you would like to install:")
                        }

                        QGCComboBox {
                            id:                 firmwareBuildTypeCombo
                            Layout.fillWidth:   true
                            visible:            showFirmwareTypeSelection
                            textRole:           "text"
                            model:              _singleFirmwareMode ? singleFirmwareModeTypeList : firmwareBuildTypeList

                            onActivated: (index) => {
                                var fwType = model.get(index).firmwareType
                                controller.selectedFirmwareBuildType = fwType
                                if (fwType === FirmwareUpgradeController.BetaFirmware) {
                                    firmwareWarningMessageVisible = true
                                    firmwareVersionWarningLabel.text = qsTr("WARNING: BETA FIRMWARE. ") +
                                            qsTr("This firmware version is ONLY intended for beta testers. ") +
                                            qsTr("Although it has received FLIGHT TESTING, it represents actively changed code. ") +
                                            qsTr("Do NOT use for normal operation.")
                                } else if (fwType === FirmwareUpgradeController.DeveloperFirmware) {
                                    firmwareWarningMessageVisible = true
                                    firmwareVersionWarningLabel.text = qsTr("WARNING: CONTINUOUS BUILD FIRMWARE. ") +
                                            qsTr("This firmware has NOT BEEN FLIGHT TESTED. ") +
                                            qsTr("It is only intended for DEVELOPERS. ") +
                                            qsTr("Run bench tests without props first. ") +
                                            qsTr("Do NOT fly this without additional safety precautions. ") +
                                            qsTr("Follow the forums actively when using it.")
                                } else {
                                    firmwareWarningMessageVisible = false
                                }
                                updatePX4VersionDisplay()
                                if (fwType === FirmwareUpgradeController.CustomFirmware) {
                                    customFirmwareDialog.openForLoad()
                                }
                            }
                        }

                        QGCLabel {
                            id:                 firmwareVersionWarningLabel
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            visible:            firmwareWarningMessageVisible
                        }
                    } // ColumnLayout
                } // QGCPopupDialog
            } // Component - firmwareSelectDialogComponent

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            !flashBootloaderButton.visible

                QGCComboBox {
                    id:                 portCombo
                    Layout.fillWidth:   true
                    visible:            !_flashStarted
                    enabled:            controller.availablePorts.length > 0
                    model:              controller.availablePorts
                    textRole:           "displayName"
                    onActivated: (index) => {
                        if (index >= 0 && index < controller.availablePorts.length) {
                            _selectedSystemLocation = controller.availablePorts[index].systemLocation
                        }
                    }
                }

                QGCLabel {
                    id:                 portLabel
                    Layout.fillWidth:   true
                    visible:            _flashStarted
                    text:               _selectedDisplayName
                    elide:              Text.ElideRight
                }

                QGCButton {
                    id:         flashButton
                    text:       qsTr("Flash")
                    visible:    !_flashStarted
                    enabled:    portCombo.currentIndex >= 0 && controller.availablePorts.length > 0
                    onClicked: {
                        var ports = controller.availablePorts
                        if (portCombo.currentIndex < 0 || portCombo.currentIndex >= ports.length) {
                            return
                        }
                        var entry = ports[portCombo.currentIndex]
                        _selectedSystemLocation = entry.systemLocation
                        _selectedDisplayName = entry.displayName
                        _flashStarted = true
                        statusTextArea.append(unplugReplugText)
                        if (QGroundControl.multiVehicleManager.activeVehicle) {
                            QGroundControl.multiVehicleManager.activeVehicle.vehicleLinkManager.autoDisconnect = true
                        }
                        controller.flashPort(entry.systemLocation)
                    }
                }

                QGCButton {
                    id:         cancelButton
                    text:       qsTr("Cancel")
                    visible:    _flashStarted
                    onClicked: {
                        controller.cancel()
                        _flashStarted = false
                        _selectedDisplayName = ""
                        statusTextArea.append(highlightPrefix + qsTr("Cancelled. Select a port and press Flash to try again.") + highlightSuffix)
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
                onClicked:  globals.activeVehicle.flashBootloader()
            }

            TextArea {
                id:                 statusTextArea
                Layout.preferredWidth:              parent.width
                Layout.fillHeight:  true
                readOnly:           true
                font.pointSize:     ScreenTools.defaultFontPointSize
                textFormat:         TextEdit.RichText
                text:               _singleFirmwareMode ? welcomeTextSingle : welcomeText
                color:              qgcPal.text

                background: Rectangle {
                    color: qgcPal.windowShade
                }
            }

        } // ColumnLayout
    } // Component
} // SetupPage
