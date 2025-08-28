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

    // Pending changes that haven't been written yet
    property var pendingValues:     ({})  // Map of setting names to values

    // The reference ESC is always the first selected ESC
    property var referenceEsc:      selectedEscs.length > 0 && escStatusModel ? escStatusModel.get(selectedEscs[0]) : null
    property var referenceFacts:     referenceEsc ? referenceEsc.am32Eeprom : null

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins:   ScreenTools.defaultFontPixelHeight / 2

    Component.onCompleted: {
        if (escStatusModel && escStatusModel.count > 0) {
            // Default: select all ESCs
            var allEscs = []
            for (var i = 0; i < escStatusModel.count; i++) {
                allEscs.push(i)
            }
            selectedEscs = allEscs

            // Request read for all ESCs to get initial data
            for (var j = 0; j < escStatusModel.count; j++) {
                if (escStatusModel.get(j).am32Eeprom) {
                    escStatusModel.get(j).am32Eeprom.requestRead(vehicle)
                }
            }
        }
    }

    function getEscBorderColor(index) {
        if (!escStatusModel || index >= escStatusModel.count) {
            return "transparent"
        }

        var escData = escStatusModel.get(index)
        if (!escData || !escData.am32Eeprom) {
            return qgcPal.colorGrey
        }

        // Check if ESC has unsaved changes
        if (escData.am32Eeprom.hasUnsavedChanges) {
            return qgcPal.colorYellow  // Yellow for pending changes
        }

        var isSelected = selectedEscs.indexOf(index) >= 0
        if (!isSelected) {
            return qgcPal.colorGrey  // Grey for unselected
        }

        // For selected ESCs, check if settings match the reference
        if (!escData.am32Eeprom.dataLoaded) {
            return qgcPal.colorRed  // Red if data not loaded
        }

        if (index === selectedEscs[0]) {
            return qgcPal.colorGreen  // First selected is always green
        }

        // Check if this ESC's settings match the reference ESC
        if (referenceEsc && referenceEsc.am32Eeprom && escData.am32Eeprom.settingsMatch(referenceEsc.am32Eeprom)) {
            return qgcPal.colorGreen  // Green if matches
        } else {
            return qgcPal.colorRed  // Red if doesn't match
        }
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

            // Reset pending values when selection changes
            pendingValues = {}
        }
    }

    function updatePendingValue(settingName, value) {
        // Update the pending value
        var newPending = Object.assign({}, pendingValues)
        newPending[settingName] = value
        pendingValues = newPending

        // Apply to all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom) {
                var changes = {}
                changes[settingName] = value
                escData.am32Eeprom.applyPendingChanges(changes)
            }
        }
    }

    function getDisplayValue(settingName) {
        // If we have a pending value, use that
        if (pendingValues.hasOwnProperty(settingName)) {
            return pendingValues[settingName]
        }

        // Otherwise use the value from the reference ESC
        if (referenceFacts) {
            return referenceFacts.getFactValue(settingName)
        }

        return null
    }

    function writeSettings() {
        // Write settings to all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom && escData.am32Eeprom.hasUnsavedChanges) {
                escData.am32Eeprom.requestWrite(vehicle)
            }
        }

        // Clear pending values after write
        pendingValues = {}
    }

    function readSettings() {
        // Read settings from all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom) {
                escData.am32Eeprom.requestRead(vehicle)
            }
        }

        // Clear pending values after read
        pendingValues = {}
    }

    function discardChanges() {
        // Discard changes on all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom) {
                escData.am32Eeprom.discardChanges()
            }
        }

        // Clear pending values
        pendingValues = {}
    }

    function hasAnyUnsavedChanges() {
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom && escData.am32Eeprom.hasUnsavedChanges) {
                return true
            }
        }
        return false
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

        // Settings Panel
        Flickable {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      settingsColumn.height
            contentWidth:       width
            clip:               true
            visible:            referenceFacts && referenceFacts.dataLoaded

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
                        ComboBox {
                            model: ["Auto", "PWM", "DShot"]
                            currentIndex: {
                                var val = getDisplayValue("inputType")
                                return val !== null ? val : 0
                            }
                            onActivated: function(index) {
                                updatePendingValue("inputType", index)
                            }
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
                        QGCCheckBox {
                            text:   qsTr("Stuck rotor protection")
                            checked: getDisplayValue("stuckRotorProtection") === true
                            onClicked: updatePendingValue("stuckRotorProtection", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Stall protection")
                            checked: getDisplayValue("antiStall") === true
                            onClicked: updatePendingValue("antiStall", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Use hall sensors")
                            checked: getDisplayValue("hallSensors") === true
                            onClicked: updatePendingValue("hallSensors", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("30ms interval telemetry")
                            checked: getDisplayValue("telemetry30ms") === true
                            onClicked: updatePendingValue("telemetry30ms", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Variable PWM")
                            checked: getDisplayValue("variablePwmFreq") === true
                            onClicked: updatePendingValue("variablePwmFreq", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Complementary PWM")
                            checked: getDisplayValue("complementaryPwm") === true
                            onClicked: updatePendingValue("complementaryPwm", checked)
                        }
                        QGCCheckBox {
                            id: autoTimingCheckbox
                            text:   qsTr("Auto timing advance")
                            checked: getDisplayValue("autoTiming") === true
                            onClicked: updatePendingValue("autoTiming", checked)
                        }

                        // Sliders with inline labels
                        Column {
                            QGCLabel { text: qsTr("Timing advance") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 30
                                majorTickStepSize:  1
                                decimalPlaces:      1
                                value:              getDisplayValue("timingAdvance") || 15
                                enabled:            !autoTimingCheckbox.checked
                                onValueChanged:     updatePendingValue("timingAdvance", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Startup power") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               50
                                to:                 150
                                majorTickStepSize:  10
                                decimalPlaces:      0
                                value:              getDisplayValue("startupPower") || 100
                                onValueChanged:     updatePendingValue("startupPower", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Motor KV") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               20
                                to:                 10220
                                majorTickStepSize:  1000
                                decimalPlaces:      0
                                value:              getDisplayValue("motorKv") || 2200
                                onValueChanged:     updatePendingValue("motorKv", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Motor poles") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2
                                to:                 36
                                majorTickStepSize:  2
                                decimalPlaces:      0
                                value:              getDisplayValue("motorPoles") || 14
                                onValueChanged:     updatePendingValue("motorPoles", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Beeper volume") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 11
                                majorTickStepSize:  1
                                decimalPlaces:      0
                                value:              getDisplayValue("beepVolume") || 5
                                onValueChanged:     updatePendingValue("beepVolume", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("PWM Frequency") }
                            ValueSlider {
                                id: pwmFreqSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               8
                                to:                 48
                                majorTickStepSize:  5
                                decimalPlaces:      0
                                value:              getDisplayValue("pwmFrequency") || 24
                                enabled:            getDisplayValue("variablePwmFreq") !== true
                                onValueChanged:     updatePendingValue("pwmFrequency", value)
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

                        QGCCheckBox {
                            text:   qsTr("Disable stick calibration")
                            checked: getDisplayValue("disableStickCalibration") === true
                            onClicked: updatePendingValue("disableStickCalibration", checked)
                        }

                        Column {
                            QGCLabel { text: qsTr("Ramp rate") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.1
                                to:                 20
                                majorTickStepSize:  2
                                decimalPlaces:      1
                                value:              getDisplayValue("maxRampSpeed") || 16.0
                                onValueChanged:     updatePendingValue("maxRampSpeed", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Minimum duty cycle") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 25
                                majorTickStepSize:  5
                                decimalPlaces:      1
                                value:              getDisplayValue("minDutyCycle") || 2.0
                                onValueChanged:     updatePendingValue("minDutyCycle", value)
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

                        QGCCheckBox {
                            id: lowVoltageCutoffCheckbox
                            text:   qsTr("Low voltage cut off")
                            checked: getDisplayValue("lowVoltageCutoff") === true
                            onClicked: updatePendingValue("lowVoltageCutoff", checked)
                        }

                        Column {
                            QGCLabel { text: qsTr("Temperature limit") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               70
                                to:                 141
                                majorTickStepSize:  10
                                decimalPlaces:      0
                                value:              getDisplayValue("temperatureLimit") || 141
                                onValueChanged:     updatePendingValue("temperatureLimit", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current limit") }
                            ValueSlider {
                                id: currentLimitSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 202
                                majorTickStepSize:  20
                                decimalPlaces:      0
                                value:              getDisplayValue("currentLimit") || 204
                                onValueChanged:     updatePendingValue("currentLimit", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Low voltage threshold") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2.5
                                to:                 3.5
                                majorTickStepSize:  0.2
                                decimalPlaces:      1
                                value:              getDisplayValue("lowVoltageThreshold") || 3.0
                                enabled:            lowVoltageCutoffCheckbox.checked
                                onValueChanged:     updatePendingValue("lowVoltageThreshold", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Absolute voltage cutoff") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.5
                                to:                 50.0
                                majorTickStepSize:  5
                                decimalPlaces:      1
                                value:              getDisplayValue("absoluteVoltageCutoff") || 5.0
                                onValueChanged:     updatePendingValue("absoluteVoltageCutoff", value)
                            }
                        }
                    }
                }

                // Current Control Group (opacity reduced when current limit > 100)
                SettingsGroupLayout {
                    heading:            qsTr("Current Control")
                    Layout.fillWidth:   true
                    opacity:            currentLimitSlider.value > 100 ? 0.3 : 1.0

                    Flow {
                        width: parent.width
                        spacing: _margins

                        Column {
                            QGCLabel { text: qsTr("Current P") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 510
                                majorTickStepSize:  50
                                decimalPlaces:      0
                                value:              getDisplayValue("currentPidP") || 200
                                onValueChanged:     updatePendingValue("currentPidP", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current I") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 255
                                majorTickStepSize:  25
                                decimalPlaces:      0
                                value:              getDisplayValue("currentPidI") || 0
                                onValueChanged:     updatePendingValue("currentPidI", value)
                            }
                        }

                        Column {
                            QGCLabel { text: qsTr("Current D") }
                            ValueSlider {
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 2550
                                majorTickStepSize:  250
                                decimalPlaces:      0
                                value:              getDisplayValue("currentPidD") || 500
                                onValueChanged:     updatePendingValue("currentPidD", value)
                            }
                        }
                    }
                }

                // Action Buttons
                RowLayout {
                    Layout.fillWidth:   true
                    layoutDirection:    Qt.RightToLeft
                    spacing:            _margins

                    QGCButton {
                        text:       qsTr("Write Settings")
                        enabled:    hasAnyUnsavedChanges() && selectedEscs.length > 0
                        highlighted: hasAnyUnsavedChanges()
                        onClicked:  writeSettings()
                    }

                    QGCButton {
                        text:       qsTr("Read Settings")
                        enabled:    selectedEscs.length > 0
                        onClicked:  readSettings()
                    }

                    QGCButton {
                        text:       qsTr("Discard Changes")
                        enabled:    hasAnyUnsavedChanges()
                        onClicked:  discardChanges()
                    }
                }
            }
        }
    }
}
