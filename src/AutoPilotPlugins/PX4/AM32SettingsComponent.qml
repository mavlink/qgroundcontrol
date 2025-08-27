import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Rectangle {
    id:                 root
    color:              qgcPal.windowShade
    implicitWidth:      mainColumn.width + (_margins * 2)
    implicitHeight:     mainColumn.height + (_margins * 2)
    radius:             ScreenTools.defaultFontPixelHeight / 2

    property var vehicle:           globals.activeVehicle
    property var escStatusModel:    vehicle ? vehicle.escs : null
    property int currentEscIndex:   0
    property var currentEsc:        null
    property var am32Facts:         currentEsc ? currentEsc.am32Eeprom : null

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    Component.onCompleted: {
        if (escStatusModel && escStatusModel.count > 0) {
            selectEsc(0)
        }
    }

    function selectEsc(index) {
        currentEscIndex = index
        currentEsc = escStatusModel.get(index)
        if (currentEsc && !currentEsc.am32Eeprom.dataLoaded) {
            requestRead()
        }
    }

    function requestRead() {
        if (currentEsc && currentEsc.am32Eeprom) {
            currentEsc.am32Eeprom.requestRead(vehicle)
        }
    }

    function requestWrite() {
        if (currentEsc && currentEsc.am32Eeprom) {
            currentEsc.am32Eeprom.requestWrite(vehicle)
        }
    }

    Column {
        id:                 mainColumn
        anchors.centerIn:   parent
        spacing:            _margins

        // Header with unsaved changes indicator
        Row {
            spacing: _margins * 2

            QGCLabel {
                text:           qsTr("AM32 ESC Configuration")
                font.pointSize: ScreenTools.mediumFontPointSize
            }

            QGCLabel {
                text:       qsTr("(unsaved changes)")
                color:      qgcPal.warningText
                visible:    am32Facts && am32Facts.hasUnsavedChanges
            }

            QGCComboBox {
                id:             escSelector
                width:          ScreenTools.defaultFontPixelWidth * 15
                model:          escStatusModel ? escStatusModel.count : 0
                currentIndex:   currentEscIndex
                displayText:    currentIndex >= 0 ? qsTr("ESC %1").arg(currentIndex + 1) : qsTr("No ESC")

                onCurrentIndexChanged: {
                    // Warn about unsaved changes before switching
                    if (am32Facts && am32Facts.hasUnsavedChanges) {
                        unsavedChangesDialog.newIndex = currentIndex
                        unsavedChangesDialog.open()
                    } else if (currentIndex >= 0) {
                        selectEsc(currentIndex)
                    }
                }
            }
        }

        // ESC Information Section
        Rectangle {
            width:      infoGrid.width + (_margins * 2)
            height:     infoGrid.height + (_margins * 2)
            color:      qgcPal.window
            radius:     ScreenTools.defaultFontPixelHeight / 4
            visible:    am32Facts && am32Facts.dataLoaded

            GridLayout {
                id:                 infoGrid
                anchors.centerIn:   parent
                columns:            2
                columnSpacing:      _margins
                rowSpacing:         _margins / 2

                QGCLabel {
                    text:       qsTr("Firmware:")
                    font.bold:  true
                }
                QGCLabel {
                    text: am32Facts ? qsTr("v%1.%2").arg(am32Facts.firmwareMajor.value).arg(am32Facts.firmwareMinor.value) : ""
                }

                QGCLabel {
                    text:       qsTr("Bootloader:")
                    font.bold:  true
                }
                QGCLabel {
                    text: am32Facts ? qsTr("v%1").arg(am32Facts.bootloaderVersion.value) : ""
                }

                QGCLabel {
                    text:       qsTr("EEPROM Version:")
                    font.bold:  true
                }
                QGCLabel {
                    text: am32Facts ? am32Facts.eepromVersion.value : ""
                }
            }
        }

        // Settings Sections
        QGCTabBar {
            id:         settingsTabBar
            visible:    am32Facts && am32Facts.dataLoaded
            width:      parent.width

            QGCTabButton { text: qsTr("Basic") }
            QGCTabButton { text: qsTr("Motor") }
            QGCTabButton { text: qsTr("Protection") }
            QGCTabButton { text: qsTr("Advanced") }
        }

        // Tab Content
        StackLayout {
            width:          ScreenTools.defaultFontPixelWidth * 50
            visible:        am32Facts && am32Facts.dataLoaded
            currentIndex:   settingsTabBar.currentIndex

            // Basic Settings Tab
            Column {
                spacing: _margins / 2

                GridLayout {
                    columns:        2
                    columnSpacing:  _margins * 2
                    rowSpacing:     _margins / 2

                    QGCLabel { text: qsTr("Motor Direction:") }
                    FactCheckBox {
                        fact: am32Facts.directionReversed
                        text: qsTr("Reversed")
                    }

                    QGCLabel { text: qsTr("Bidirectional Mode:") }
                    FactCheckBox {
                        fact: am32Facts.bidirectionalMode
                    }

                    QGCLabel { text: qsTr("Brake on Stop:") }
                    FactCheckBox {
                        fact: am32Facts.brakeOnStop
                    }

                    QGCLabel { text: qsTr("Beep Volume:") }
                    Row {
                        spacing: _margins / 2
                        FactSlider {
                            fact:   am32Facts.beepVolume
                            from:   0
                            to:     11
                            width:  ScreenTools.defaultFontPixelWidth * 20
                            majorTickStepSize: 1
                        }
                        FactLabel {
                            fact: am32Facts.beepVolume
                        }
                    }
                }
            }

            // Motor Settings Tab
            Column {
                spacing: _margins / 2

                GridLayout {
                    columns:        2
                    columnSpacing:  _margins * 2
                    rowSpacing:     _margins / 2

                    QGCLabel { text: qsTr("Motor KV:") }
                    FactTextField {
                        fact:       am32Facts.motorKv
                        showUnits:  true
                    }

                    QGCLabel { text: qsTr("Motor Poles:") }
                    FactTextField {
                        fact: am32Facts.motorPoles
                    }

                    QGCLabel { text: qsTr("Timing Advance:") }
                    Row {
                        spacing: _margins / 2
                        FactSlider {
                            fact:   am32Facts.timingAdvance
                            from:   0
                            to:     30
                            width:  ScreenTools.defaultFontPixelWidth * 20
                            majorTickStepSize: 1
                        }
                        FactLabel {
                            fact:       am32Facts.timingAdvance
                            showUnits:  true
                        }
                    }

                    QGCLabel { text: qsTr("PWM Frequency:") }
                    FactTextField {
                        fact:       am32Facts.pwmFrequency
                        showUnits:  true
                    }

                    QGCLabel { text: qsTr("Startup Power:") }
                    Row {
                        spacing: _margins / 2
                        FactSlider {
                            fact:   am32Facts.startupPower
                            from:   50
                            to:     150
                            width:  ScreenTools.defaultFontPixelWidth * 20
                            majorTickStepSize: 1
                        }
                        FactLabel {
                            fact:       am32Facts.startupPower
                            showUnits:  true
                        }
                    }

                    QGCLabel { text: qsTr("Min Duty Cycle:") }
                    FactTextField {
                        fact:       am32Facts.minDutyCycle
                        showUnits:  true
                    }

                    QGCLabel { text: qsTr("Max Ramp Speed:") }
                    FactTextField {
                        fact:       am32Facts.maxRampSpeed
                        showUnits:  true
                    }
                }
            }

            // Protection Settings Tab
            Column {
                spacing: _margins / 2

                GridLayout {
                    columns:        2
                    columnSpacing:  _margins * 2
                    rowSpacing:     _margins / 2

                    QGCLabel { text: qsTr("Temperature Limit:") }
                    FactTextField {
                        fact:       am32Facts.temperatureLimit
                        showUnits:  true
                    }

                    QGCLabel { text: qsTr("Current Limit:") }
                    FactTextField {
                        fact:       am32Facts.currentLimit
                        showUnits:  true
                    }

                    QGCLabel { text: qsTr("Low Voltage Cutoff:") }
                    FactCheckBox {
                        fact: am32Facts.lowVoltageCutoff
                    }

                    QGCLabel {
                        text:       qsTr("Low Voltage Threshold:")
                        enabled:    am32Facts.lowVoltageCutoff.value
                    }
                    FactTextField {
                        fact:       am32Facts.lowVoltageThreshold
                        showUnits:  true
                        enabled:    am32Facts.lowVoltageCutoff.value
                    }

                    QGCLabel { text: qsTr("Anti-Stall Protection:") }
                    FactCheckBox {
                        fact: am32Facts.antiStall
                    }

                    QGCLabel { text: qsTr("Stuck Rotor Protection:") }
                    FactCheckBox {
                        fact: am32Facts.stuckRotorProtection
                    }
                }
            }

            // Advanced Settings Tab
            Column {
                spacing: _margins / 2

                GridLayout {
                    columns:        2
                    columnSpacing:  _margins * 2
                    rowSpacing:     _margins / 2

                    QGCLabel { text: qsTr("Sine Startup:") }
                    FactCheckBox {
                        fact:       am32Facts.sineStartup
                        enabled:    am32Facts.complementaryPwm.value
                    }

                    QGCLabel { text: qsTr("Complementary PWM:") }
                    FactCheckBox {
                        fact: am32Facts.complementaryPwm
                    }

                    QGCLabel { text: qsTr("Variable PWM Freq:") }
                    FactCheckBox {
                        fact: am32Facts.variablePwmFreq
                    }

                    QGCLabel { text: qsTr("30ms Telemetry:") }
                    FactCheckBox {
                        fact: am32Facts.telemetry30ms
                    }

                    QGCLabel { text: qsTr("Input Type:") }
                    FactComboBox {
                        fact:           am32Facts.inputType
                        indexModel:     false
                        sizeToContents: true
                    }

                    QGCLabel { text: qsTr("Auto Timing:") }
                    FactCheckBox {
                        fact: am32Facts.autoTiming
                    }

                    QGCLabel { text: qsTr("Current PID P:") }
                    FactTextField {
                        fact: am32Facts.currentPidP
                    }

                    QGCLabel { text: qsTr("Current PID I:") }
                    FactTextField {
                        fact: am32Facts.currentPidI
                    }

                    QGCLabel { text: qsTr("Current PID D:") }
                    FactTextField {
                        fact: am32Facts.currentPidD
                    }
                }
            }
        }

        // Action Buttons
        Row {
            spacing:            _margins / 4
            layoutDirection:    Qt.RightToLeft
            width:              parent.width

            QGCButton {
                text:       qsTr("Write")
                enabled:    am32Facts && am32Facts.dataLoaded && am32Facts.hasUnsavedChanges
                highlighted: am32Facts && am32Facts.hasUnsavedChanges

                onClicked:  {
                    writeConfirmDialog.open()
                }
            }

            QGCButton {
                text:       qsTr("Read")
                onClicked:  {
                    if (am32Facts && am32Facts.hasUnsavedChanges) {
                        reloadConfirmDialog.open()
                    } else {
                        requestRead()
                    }
                }
            }

            QGCButton {
                text:       qsTr("Discard")
                enabled:    am32Facts && am32Facts.hasUnsavedChanges
                onClicked:  {
                    am32Facts.discardChanges()
                }
            }

            QGCButton {
                text:       qsTr("Update Firmware")
                enabled:    false
                onClicked:  {
                    console.log("Firmware update not yet implemented")
                }
            }
        }

        // Dialogs
        MessageDialog {
            id:         writeConfirmDialog
            title:      qsTr("Write Settings")
            text:       qsTr("Are you sure you want to write these settings to ESC %1?").arg(currentEscIndex + 1)
            buttons:    MessageDialog.Yes | MessageDialog.No

            onAccepted: {
                requestWrite()
            }
        }

        MessageDialog {
            id:         unsavedChangesDialog
            title:      qsTr("Unsaved Changes")
            text:       qsTr("ESC %1 has unsaved changes. Do you want to save them first?").arg(currentEscIndex + 1)
            buttons:    MessageDialog.Save | MessageDialog.Discard | MessageDialog.Cancel

            property int newIndex: -1

            onAccepted: {
                // Save was selected
                requestWrite()
                // Wait for write to complete, then switch
                writeCompleteConnection.enabled = true
            }

            onRejected: {
                // Discard was selected
                am32Facts.discardChanges()
                if (newIndex >= 0) {
                    selectEsc(newIndex)
                }
            }
        }

        MessageDialog {
            id:         reloadConfirmDialog
            title:      qsTr("Reload Settings")
            text:       qsTr("This will discard any unsaved changes. Continue?")
            buttons:    MessageDialog.Yes | MessageDialog.No

            onAccepted: {
                requestRead()
            }
        }

        // Connections
        Connections {
            id:         writeCompleteConnection
            target:     am32Facts
            enabled:    false

            function onWriteComplete(success) {
                if (success) {
                    am32Facts.markChangesSaved()

                    // If we were switching ESCs after save, do it now
                    if (unsavedChangesDialog.newIndex >= 0) {
                        selectEsc(unsavedChangesDialog.newIndex)
                        unsavedChangesDialog.newIndex = -1
                    }
                }
                enabled = false
            }
        }

        Connections {
            target: am32Facts

            function onReadComplete(success) {
                if (success) {
                    console.log("AM32 settings read successfully for ESC " + currentEscIndex)
                } else {
                    console.log("Failed to read AM32 settings for ESC " + currentEscIndex)
                }
            }

            function onWriteComplete(success) {
                if (success) {
                    console.log("AM32 settings written successfully for ESC " + currentEscIndex)
                    // Automatically read back to verify
                    requestRead()
                } else {
                    console.log("Failed to write AM32 settings for ESC " + currentEscIndex)
                }
            }
        }
    }
}
