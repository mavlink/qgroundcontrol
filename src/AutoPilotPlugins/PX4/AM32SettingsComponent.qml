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
        if (!escData) {
            return "transparent"
        }

        // Check if ESC has unsaved changes (purple border)
        if (escData.am32Eeprom && escData.am32Eeprom.hasUnsavedChanges) {
            return qgcPal.brandingPurple || "#8B008B"
        }

        // Check if ESC is selected (green border)
        if (selectedEscs.indexOf(index) >= 0) {
            return qgcPal.brandingGreen || "#00AA00"
        }

        // Check if ESC has mismatched settings or is missing (red border)
        if (!escData.am32Eeprom || !escData.am32Eeprom.dataLoaded) {
            return qgcPal.warningText || "#FF4444"
        }

        // Not selected (no border)
        return "transparent"
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
                    height:         ScreenTools.defaultFontPixelHeight * 6
                    radius:         ScreenTools.defaultFontPixelHeight / 4
                    border.width:   3
                    border.color:   getEscBorderColor(index)
                    color:          qgcPal.window

                    property var escData: escStatusModel ? escStatusModel.get(index) : null
                    property bool isLoading: escData && escData.am32Eeprom && escData.am32Eeprom.hasOwnProperty("isLoading") ? escData.am32Eeprom.isLoading : false

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

                        // Loading indicator
                        QGCColoredImage {
                            source:                 "/InstrumentValueIcons/refresh.svg"
                            height:                 ScreenTools.defaultFontPixelHeight
                            width:                  height
                            sourceSize.height:      height
                            color:                  qgcPal.text
                            visible:                parent.parent.isLoading
                            anchors.horizontalCenter: parent.horizontalCenter

                            RotationAnimation on rotation {
                                loops:      Animation.Infinite
                                from:       0
                                to:         360
                                duration:   1000
                                running:    parent.visible
                            }
                        }

                        // ESC Info when not loading
                        Column {
                            visible: !parent.parent.isLoading
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

        // Info text
        QGCLabel {
            Layout.alignment: Qt.AlignHCenter
            text: {
                if (selectedEscs.length === 0) {
                    return qsTr("Click an ESC to select it")
                } else if (selectedEscs.length === 1) {
                    return qsTr("Settings for ESC %1").arg(selectedEscs[0] + 1)
                } else if (selectedEscs.length === escStatusModel.count) {
                    return qsTr("Settings for all ESCs")
                } else {
                    return qsTr("Settings for %1 ESCs").arg(selectedEscs.length)
                }
            }
            font.italic:        true
            visible:            escStatusModel && escStatusModel.count > 0
        }

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

                    GridLayout {
                        columns:        2
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins

                        QGCLabel { text: qsTr("Protocol:") }
                        FactComboBox {
                            fact:           am32Facts.inputType
                            indexModel:     false
                            sizeToContents: true
                            Layout.fillWidth: true
                        }
                    }
                }

                // Motor Group
                SettingsGroupLayout {
                    heading: qsTr("Motor")
                    Layout.fillWidth: true

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins
                        rowSpacing:     _groupMargins

                        // Switches
                        FactCheckBox {
                            text:               qsTr("Stuck rotor protection")
                            fact:               am32Facts.stuckRotorProtection
                        }

                        FactCheckBox {
                            text:               qsTr("Stall protection")
                            fact:               am32Facts.antiStall
                        }

                        FactCheckBox {
                            text:               qsTr("Use hall sensors")
                            fact:               am32Facts.hallSensors
                        }

                        FactCheckBox {
                            text:               qsTr("30ms interval telemetry")
                            fact:               am32Facts.telemetry30ms
                        }

                        FactCheckBox {
                            text:               qsTr("Variable PWM")
                            fact:               am32Facts.variablePwmFreq
                        }

                        FactCheckBox {
                            text:               qsTr("Complementary PWM")
                            fact:               am32Facts.complementaryPwm
                        }

                        // Sliders - use grid columns properly
                        QGCLabel { text: qsTr("Timing advance") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 30
                            fact:               am32Facts.timingAdvance
                            majorTickStepSize:  1
                            enabled:            !am32Facts.autoTiming.value
                        }

                        QGCLabel { text: qsTr("Startup power") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               50
                            to:                 150
                            fact:               am32Facts.startupPower
                            majorTickStepSize:  1
                        }

                        QGCLabel { text: qsTr("Motor KV") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               20
                            to:                 10220
                            fact:               am32Facts.motorKv
                            majorTickStepSize:  500
                        }

                        QGCLabel { text: qsTr("Motor poles") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               2
                            to:                 36
                            fact:               am32Facts.motorPoles
                            majorTickStepSize:  2
                        }

                        QGCLabel { text: qsTr("Beeper volume") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 11
                            fact:               am32Facts.beepVolume
                            majorTickStepSize:  1
                        }

                        QGCLabel { text: qsTr("PWM Frequency") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               8
                            to:                 48
                            fact:               am32Facts.pwmFrequency
                            enabled:            am32Facts.variablePwmFreq.value < 2
                            majorTickStepSize:  5
                        }
                    }
                }

                // Extended Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Extended Settings")
                    Layout.fillWidth: true

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins
                        rowSpacing:     _groupMargins

                        FactCheckBox {
                            text:               qsTr("Disable stick calibration")
                            fact:               am32Facts.disableStickCalibration
                        }

                        Item { Layout.fillWidth: true }
                        Item { Layout.fillWidth: true }

                        QGCLabel { text: qsTr("Ramp rate") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0.1
                            to:                 20
                            fact:               am32Facts.maxRampSpeed
                            majorTickStepSize:  2
                        }

                        QGCLabel { text: qsTr("Minimum duty cycle") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 25
                            fact:               am32Facts.minDutyCycle
                            majorTickStepSize:  5
                        }
                    }
                }

                // Limits Group
                SettingsGroupLayout {
                    heading: qsTr("Limits")
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: _groupMargins

                        FactCheckBox {
                            text:   qsTr("Low voltage cut off")
                            fact:   am32Facts.lowVoltageCutoff
                        }

                        GridLayout {
                            columns:        3
                            columnSpacing:  _margins
                            rowSpacing:     _groupMargins

                            QGCLabel { text: qsTr("Temperature limit") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               70
                                to:                 141
                                fact:               am32Facts.temperatureLimit
                                majorTickStepSize:  10
                            }

                            QGCLabel { text: qsTr("Current limit") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               0
                                to:                 202
                                fact:               am32Facts.currentLimit
                                majorTickStepSize:  20
                            }

                            QGCLabel { text: qsTr("Low voltage threshold") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               2.5
                                to:                 3.5
                                fact:               am32Facts.lowVoltageThreshold
                                enabled:            am32Facts.lowVoltageCutoff.value
                                majorTickStepSize:  0.1
                            }

                            QGCLabel { text: qsTr("Absolute voltage cutoff") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               0.5
                                to:                 50.0
                                fact:               am32Facts.absoluteVoltageCutoff
                                majorTickStepSize:  5
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroupLayout {
                    heading:            qsTr("Current Control")
                    Layout.fillWidth:   true
                    opacity:            am32Facts.currentLimit.value > 100 ? 0.3 : 1.0

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins
                        rowSpacing:     _groupMargins

                        QGCLabel { text: qsTr("Current P") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 510
                            fact:               am32Facts.currentPidP
                            majorTickStepSize:  50
                        }

                        QGCLabel { text: qsTr("Current I") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 255
                            fact:               am32Facts.currentPidI
                            majorTickStepSize:  25
                        }

                        QGCLabel { text: qsTr("Current D") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 2550
                            fact:               am32Facts.currentPidD
                            majorTickStepSize:  250
                        }
                    }
                }

                // Sinusoidal Startup Group
                SettingsGroupLayout {
                    heading: qsTr("Sinusoidal Startup")
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: _groupMargins

                        FactCheckBox {
                            text:   qsTr("Sinusoidal startup")
                            fact:   am32Facts.sineStartup
                        }

                        GridLayout {
                            columns:        3
                            columnSpacing:  _margins
                            rowSpacing:     _groupMargins

                            QGCLabel { text: qsTr("Sine mode range") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               5
                                to:                 25
                                fact:               am32Facts.sineModeRange
                                enabled:            am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                                majorTickStepSize:  5
                            }

                            QGCLabel { text: qsTr("Sine mode power") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               1
                                to:                 10
                                fact:               am32Facts.sineModeStrength
                                enabled:            am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                                majorTickStepSize:  1
                            }
                        }
                    }
                }

                // Brake Group
                SettingsGroupLayout {
                    heading: qsTr("Brake")
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: _groupMargins

                        RowLayout {
                            spacing: _margins * 2

                            FactCheckBox {
                                text:   qsTr("Brake on stop")
                                fact:   am32Facts.brakeOnStop
                            }

                            FactCheckBox {
                                text:   qsTr("RC car reversing")
                                fact:   am32Facts.rcCarReversing
                            }
                        }

                        GridLayout {
                            columns:        3
                            columnSpacing:  _margins
                            rowSpacing:     _groupMargins

                            QGCLabel { text: qsTr("Brake strength") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               1
                                to:                 10
                                fact:               am32Facts.dragBrakeStrength
                                enabled:            am32Facts.brakeOnStop.value && !am32Facts.rcCarReversing.value
                                majorTickStepSize:  1
                            }

                            QGCLabel { text: qsTr("Running brake level") }
                            FactSlider {
                                Layout.columnSpan:  2
                                Layout.fillWidth:   true
                                from:               1
                                to:                 10
                                fact:               am32Facts.runningBrakeAmount
                                enabled:            !am32Facts.rcCarReversing.value
                                majorTickStepSize:  1
                            }
                        }
                    }
                }

                // Servo Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Servo Settings")
                    Layout.fillWidth: true

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins
                        rowSpacing:     _groupMargins

                        QGCLabel { text: qsTr("Low threshold") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               750
                            to:                 1250
                            fact:               am32Facts.servoLowThreshold
                            majorTickStepSize:  50
                        }

                        QGCLabel { text: qsTr("High threshold") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               1750
                            to:                 2250
                            fact:               am32Facts.servoHighThreshold
                            majorTickStepSize:  50
                        }

                        QGCLabel { text: qsTr("Neutral") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               1374
                            to:                 1630
                            fact:               am32Facts.servoNeutral
                            majorTickStepSize:  25
                        }

                        QGCLabel { text: qsTr("Dead band") }
                        FactSlider {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            from:               0
                            to:                 100
                            fact:               am32Facts.servoDeadband
                            majorTickStepSize:  10
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
    Item {
        anchors.fill: parent
        visible: !am32Facts || !am32Facts.dataLoaded

        Column {
            anchors.centerIn: parent
            spacing: _margins

            QGCLabel {
                text: qsTr("No ESC data available")
                font.bold: true
                font.pointSize: ScreenTools.largeFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            QGCLabel {
                text: qsTr("Connect an ESC with AM32 firmware to configure settings.")
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.WordWrap
                width: root.width * 0.8
            }

            QGCButton {
                text: qsTr("Request ESC Data")
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: escStatusModel && escStatusModel.count > 0
                onClicked: {
                    if (escStatusModel && escStatusModel.count > 0) {
                        for (var i = 0; i < escStatusModel.count; i++) {
                            var escData = escStatusModel.get(i)
                            if (escData && escData.am32Eeprom) {
                                escData.am32Eeprom.requestRead(vehicle)
                            }
                        }
                    }
                }
            }
        }
    }
}
