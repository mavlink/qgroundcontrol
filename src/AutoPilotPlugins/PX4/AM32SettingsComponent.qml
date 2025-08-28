import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:                 root

    property var vehicle:           globals.activeVehicle
    property var escStatusModel:    vehicle ? vehicle.escs : null
    property var selectedEscs:      []  // Array of selected ESC indices
    property var am32Facts:         selectedEscs.length > 0 && escStatusModel ? escStatusModel.get(selectedEscs[0]).am32Eeprom : null

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins:   ScreenTools.defaultFontPixelHeight / 2

    Component.onCompleted: {
        if (escStatusModel && escStatusModel.count > 0) {
            // Default: select all ESCs (matching AM32 Configurator behavior)
            var allEscs = []
            for (var i = 0; i < escStatusModel.count; i++) {
                allEscs.push(i)
            }
            selectedEscs = allEscs

            // Request read for first ESC to get initial data
            if (escStatusModel.get(0).am32Eeprom) {
                escStatusModel.get(0).am32Eeprom.requestRead(vehicle)
            }
        }
    }

    function getEscBorderColor(index) {
        if (!escStatusModel || index >= escStatusModel.count) {
            return "transparent"
        }

        var escData = escStatusModel.get(index)

        // TODO: something else?
        if (!escData) {
            return qgcPal.colorGrey
        }

        // Check if ESC has unsaved changes
        if (escData.am32Eeprom && escData.am32Eeprom.hasUnsavedChanges) {
            return qgcPal.colorYellow
        }

        // Check if ESC is selected
        if (selectedEscs.indexOf(index) >= 0) {
            return qgcPal.colorGreen
        }

        // Check if ESC has mismatched settings or is missing
        if (!escData.am32Eeprom || !escData.am32Eeprom.dataLoaded) {
            return qgcPal.colorRed
        }

        return qgcPal.colorGrey
    }

    function toggleEscSelection(index) {
        var idx = selectedEscs.indexOf(index)
        var newSelection = selectedEscs.slice()

        if (idx >= 0) {
            // Deselect ESC
            newSelection.splice(idx, 1)
        } else {
            // Select ESC
            newSelection.push(index)
            newSelection.sort(function(a, b) { return a - b })
        }

        selectedEscs = newSelection

        // Load data for first selected ESC if needed
        if (selectedEscs.length > 0) {
            var firstSelected = escStatusModel.get(selectedEscs[0])
            if (firstSelected.am32Eeprom && !firstSelected.am32Eeprom.dataLoaded) {
                firstSelected.am32Eeprom.requestRead(vehicle)
            }
        }
    }

    // Main layout using ColumnLayout for proper spacing
    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    _margins
        spacing:            _margins

        // ESC Selection Header
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: _margins / 2

            Repeater {
                model: escStatusModel ? escStatusModel.count : 0

                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth * 12
                    height:         ScreenTools.defaultFontPixelHeight * 4
                    radius:         ScreenTools.defaultFontPixelHeight / 4
                    border.width:   3
                    border.color:   getEscBorderColor(index)
                    color:          qgcPal.window

                    property var escData: escStatusModel ? escStatusModel.get(index) : null

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toggleEscSelection(index)
                        cursorShape: Qt.PointingHandCursor
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        QGCLabel {
                            text:                   qsTr("ESC %1").arg(index + 1)
                            font.bold:              true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Column {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 2

                            QGCLabel {
                                text:                   (escData && escData.am32Eeprom && escData.am32Eeprom.bootloaderVersion) ?
                                                      "BL: v" + escData.am32Eeprom.bootloaderVersion.value
                                                      : "---"
                                font.pointSize:         ScreenTools.smallFontPointSize
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            QGCLabel {
                                text:                   (escData && escData.am32Eeprom && escData.am32Eeprom.firmwareMajor && escData.am32Eeprom.firmwareMinor) ?
                                                      "FW: v" + escData.am32Eeprom.firmwareMajor.value +
                                                      "." + escData.am32Eeprom.firmwareMinor.value
                                                      : "---"
                                font.pointSize:         ScreenTools.smallFontPointSize
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }
        }

        // // Info text
        // QGCLabel {
        //     Layout.alignment: Qt.AlignHCenter
        //     text: {
        //         if (selectedEscs.length === 0) {
        //             return qsTr("Click an ESC to select it")
        //         } else if (selectedEscs.length === 1) {
        //             return qsTr("Settings for ESC %1").arg(selectedEscs[0] + 1)
        //         } else if (selectedEscs.length === escStatusModel.count) {
        //             return qsTr("Settings for all ESCs")
        //         } else {
        //             return qsTr("Settings for %1 ESCs").arg(selectedEscs.length)
        //         }
        //     }
        //     font.italic:        true
        //     visible:            escStatusModel && escStatusModel.count > 0
        // }

        // Settings Panel
        Flickable {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      settingsColumn.height
            contentWidth:       width
            clip:               true
            visible:            am32Facts && am32Facts.dataLoaded

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            ColumnLayout {
                id:         settingsColumn
                width:      parent.width
                spacing:    _margins

                // Essentials Group
                SettingsGroupLayout {
                    heading: qsTr("Essentials")
                    Layout.fillWidth: true

                    RowLayout {
                        QGCLabel { text: qsTr("Protocol:") }
                        FactComboBox {
                            fact:           am32Facts.inputType
                            indexModel:     false
                            sizeToContents: true
                        }
                    }
                }

                // Motor Group
                SettingsGroupLayout {
                    heading: qsTr("Motor")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        // Row 1 - Checkboxes
                        FactCheckBox {
                            text:   qsTr("Stuck rotor protection")
                            fact:   am32Facts.stuckRotorProtection
                        }
                        FactCheckBox {
                            text:   qsTr("Stall protection")
                            fact:   am32Facts.antiStall
                        }
                        FactCheckBox {
                            text:   qsTr("Use hall sensors")
                            fact:   am32Facts.hallSensors
                        }
                        FactCheckBox {
                            text:   qsTr("30ms interval telemetry")
                            fact:   am32Facts.telemetry30ms
                        }
                        FactCheckBox {
                            text:   qsTr("Variable PWM")
                            fact:   am32Facts.variablePwmFreq
                        }
                        FactCheckBox {
                            text:   qsTr("Complementary PWM")
                            fact:   am32Facts.complementaryPwm
                        }
                        FactCheckBox {
                            text:   qsTr("Auto timing advance")
                            fact:   am32Facts.autoTiming
                        }

                        // Sliders with inline labels
                        Column {
                            QGCLabel { text: qsTr("Timing advance") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 30
                                majorTickStepSize:  1
                                fact:               am32Facts.timingAdvance
                                enabled:            !am32Facts.autoTiming.value
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Startup power") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               50
                                to:                 150
                                majorTickStepSize:  1
                                fact:               am32Facts.startupPower
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Motor KV") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               20
                                to:                 10220
                                majorTickStepSize:  40
                                fact:               am32Facts.motorKv
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Motor poles") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2
                                to:                 36
                                majorTickStepSize:  2
                                fact:               am32Facts.motorPoles
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Beeper volume") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 11
                                majorTickStepSize:  1
                                fact:               am32Facts.beepVolume
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("PWM Frequency") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               8
                                to:                 48
                                majorTickStepSize:  1
                                fact:               am32Facts.pwmFrequency
                                enabled:            am32Facts.variablePwmFreq.value < 2
                            }
                        }
                    }
                }

                // Extended Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Extended Settings")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        FactCheckBox {
                            text:   qsTr("Disable stick calibration")
                            fact:   am32Facts.disableStickCalibration
                        }

                        Column {
                            QGCLabel { text: qsTr("Ramp rate") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.1
                                to:                 20
                                majorTickStepSize:  0.1
                                fact:               am32Facts.maxRampSpeed
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Minimum duty cycle") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 25
                                majorTickStepSize:  0.5
                                fact:               am32Facts.minDutyCycle
                            }
                        }
                    }
                }

                // Limits Group
                SettingsGroupLayout {
                    heading: qsTr("Limits")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        FactCheckBox {
                            text:   qsTr("Low voltage cut off")
                            fact:   am32Facts.lowVoltageCutoff
                        }

                        Column {
                            QGCLabel { text: qsTr("Temperature limit") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               70
                                to:                 141
                                majorTickStepSize:  1
                                fact:               am32Facts.temperatureLimit
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current limit") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 202
                                majorTickStepSize:  2
                                fact:               am32Facts.currentLimit
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Low voltage threshold") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2.5
                                to:                 3.5
                                majorTickStepSize:  0.1
                                fact:               am32Facts.lowVoltageThreshold
                                enabled:            am32Facts.lowVoltageCutoff.value
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Absolute voltage cutoff") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.5
                                to:                 50.0
                                majorTickStepSize:  0.5
                                fact:               am32Facts.absoluteVoltageCutoff
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroupLayout {
                    heading:            qsTr("Current Control")
                    Layout.fillWidth:   true
                    opacity:            am32Facts.currentLimit.value > 100 ? 0.3 : 1.0

                    Flow {
                        width: parent.width
                        spacing: _margins

                        Column {
                            QGCLabel { text: qsTr("Current P") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 510
                                majorTickStepSize:  10
                                fact:               am32Facts.currentPidP
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current I") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 255
                                majorTickStepSize:  10
                                fact:               am32Facts.currentPidI
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current D") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 2550
                                majorTickStepSize:  10
                                fact:               am32Facts.currentPidD
                            }
                        }
                    }
                }

                // Sinusoidal Startup Group
                SettingsGroupLayout {
                    heading: qsTr("Sinusoidal Startup")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        FactCheckBox {
                            text:   qsTr("Sinusoidal startup")
                            fact:   am32Facts.sineStartup
                        }

                        Column {
                            QGCLabel { text: qsTr("Sine mode range") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               5
                                to:                 25
                                majorTickStepSize:  1
                                fact:               am32Facts.sineModeRange
                                enabled:            am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Sine mode power") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               1
                                to:                 10
                                majorTickStepSize:  1
                                fact:               am32Facts.sineModeStrength
                                enabled:            am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                            }
                        }
                    }
                }

                // Brake Group
                SettingsGroupLayout {
                    heading: qsTr("Brake")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        FactCheckBox {
                            text:   qsTr("Brake on stop")
                            fact:   am32Facts.brakeOnStop
                        }

                        FactCheckBox {
                            text:   qsTr("RC car reversing")
                            fact:   am32Facts.rcCarReversing
                        }

                        Column {
                            QGCLabel { text: qsTr("Brake strength") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               1
                                to:                 10
                                majorTickStepSize:  1
                                fact:               am32Facts.dragBrakeStrength
                                enabled:            am32Facts.brakeOnStop.value && !am32Facts.rcCarReversing.value
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Running brake level") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               1
                                to:                 10
                                majorTickStepSize:  1
                                fact:               am32Facts.runningBrakeAmount
                                enabled:            !am32Facts.rcCarReversing.value
                            }
                        }
                    }
                }

                // Servo Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Servo Settings")
                    Layout.fillWidth: true

                    Flow {
                        width: parent.width
                        spacing: _margins

                        Column {
                            QGCLabel { text: qsTr("Low threshold") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               750
                                to:                 1250
                                majorTickStepSize:  1
                                fact:               am32Facts.servoLowThreshold
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("High threshold") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               1750
                                to:                 2250
                                majorTickStepSize:  1
                                fact:               am32Facts.servoHighThreshold
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Neutral") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               1374
                                to:                 1630
                                majorTickStepSize:  1
                                fact:               am32Facts.servoNeutral
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Dead band") }
                            FactSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 100
                                majorTickStepSize:  1
                                fact:               am32Facts.servoDeadband
                            }
                        }
                    }
                }
            }
        }

        // Action Buttons
        RowLayout {
            Layout.fillWidth:   true
            layoutDirection:    Qt.RightToLeft
            spacing:            _margins
            visible:            am32Facts && am32Facts.dataLoaded

            QGCButton {
                text:       qsTr("Write Settings")
                enabled:    am32Facts && am32Facts.hasUnsavedChanges && selectedEscs.length > 0
                highlighted: am32Facts && am32Facts.hasUnsavedChanges
                onClicked:  {
                    // Write settings to all selected ESCs
                    for (var i = 0; i < selectedEscs.length; i++) {
                        var escData = escStatusModel.get(selectedEscs[i])
                        if (escData.am32Eeprom) {
                            escData.am32Eeprom.requestWrite(vehicle)
                        }
                    }
                }
            }

            QGCButton {
                text:       qsTr("Read Settings")
                enabled:    selectedEscs.length > 0
                onClicked:  {
                    for (var i = 0; i < selectedEscs.length; i++) {
                        var escData = escStatusModel.get(selectedEscs[i])
                        if (escData.am32Eeprom) {
                            escData.am32Eeprom.requestRead(vehicle)
                        }
                    }
                }
            }

            QGCButton {
                text:       qsTr("Reset to Defaults")
                enabled:    am32Facts && selectedEscs.length > 0
                onClicked:  {
                    // TODO: Implement reset to defaults
                }
            }
        }
    }

    // Status message overlay when no ESC data available
    // Item {
    //     anchors.fill: parent
    //     visible: !am32Facts || !am32Facts.dataLoaded

    //     Column {
    //         anchors.centerIn: parent
    //         spacing: _margins

    //         QGCLabel {
    //             text: qsTr("No ESC data available")
    //             font.bold: true
    //             font.pointSize: ScreenTools.largeFontPointSize
    //             anchors.horizontalCenter: parent.horizontalCenter
    //         }

    //         QGCLabel {
    //             text: qsTr("Connect an ESC with AM32 firmware to configure settings.")
    //             anchors.horizontalCenter: parent.horizontalCenter
    //             wrapMode: Text.WordWrap
    //             width: root.width * 0.8
    //         }

    //         QGCButton {
    //             text: qsTr("Request ESC Data")
    //             anchors.horizontalCenter: parent.horizontalCenter
    //             enabled: escStatusModel && escStatusModel.count > 0
    //             onClicked: {
    //                 if (escStatusModel && escStatusModel.count > 0) {
    //                     for (var i = 0; i < escStatusModel.count; i++) {
    //                         var escData = escStatusModel.get(i)
    //                         if (escData && escData.am32Eeprom) {
    //                             escData.am32Eeprom.requestRead(vehicle)
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
}
