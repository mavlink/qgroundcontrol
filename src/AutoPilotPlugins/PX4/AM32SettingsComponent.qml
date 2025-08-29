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

    // Track which ESCs have loaded data
    property var loadedEscs:        new Set()
    property bool allDataLoaded:    false
    property bool initializationComplete: false

    // Timer for timeout mechanism
    Timer {
        id: dataLoadTimeout
        interval: 3000  // 3 second timeout
        repeat: false
        onTriggered: {
            console.info("Data load timeout - initializing UI with available data")
            finalizeInitialization()
        }
    }

    Component.onCompleted: {
        if (escStatusModel && escStatusModel.count > 0) {
            // Default: select all ESCs
            var allEscs = []
            for (var i = 0; i < escStatusModel.count; i++) {
                allEscs.push(i)
            }
            selectedEscs = allEscs

            // Start timeout timer
            dataLoadTimeout.start()

            // Request read for all ESCs to get initial data
            for (var j = 0; j < escStatusModel.count; j++) {
                var esc = escStatusModel.get(j)
                if (esc && esc.am32Eeprom) {
                    console.info("Requesting EEPROM data for ESC " + (j + 1))

                    // Set up connection to monitor data loading for this ESC
                    setupEscDataConnection(j)

                    // Request the data
                    esc.am32Eeprom.requestReadAll(vehicle)
                }
            }
        } else {
            // No ESCs available, complete initialization immediately
            initializationComplete = true
        }
    }

    function setupEscDataConnection(escIndex) {
        var esc = escStatusModel.get(escIndex)
        if (!esc || !esc.am32Eeprom) return

        // Create a connection to monitor when this ESC's data loads
        var conn = esc.am32Eeprom.dataLoadedChanged.connect(function() {
            if (esc.am32Eeprom.dataLoaded) {
                console.info("ESC " + (escIndex + 1) + " data loaded")
                loadedEscs.add(escIndex)

                // Check if all ESCs have loaded
                if (loadedEscs.size === escStatusModel.count) {
                    console.info("All ESC data loaded")
                    dataLoadTimeout.stop()
                    finalizeInitialization()
                }
            }
        })
    }

    function finalizeInitialization() {
        if (!initializationComplete) {
            console.info("Finalizing initialization with " + loadedEscs.size + "/" + escStatusModel.count + " ESCs loaded")
            allDataLoaded = loadedEscs.size === escStatusModel.count
            initializationComplete = true

            // Initialize sliders and checkboxes with loaded data
            pendingValues = {}
            updateSliderValues()
        }
    }

    // Listen for EEPROM data updates after initialization
    Connections {
        target: referenceFacts
        enabled: initializationComplete  // Only listen after initialization
        function onDataLoadedChanged() {
            if (initializationComplete && referenceFacts.dataLoaded) {
                console.info("Reference ESC data updated")
                pendingValues = {}
                updateSliderValues()
            }
        }
    }

    function updateSliderValues() {
        // Only update sliders after initialization is complete
        if (!initializationComplete) {
            return
        }

        // Update all slider values when data changes externally
        if (timingAdvanceSlider) timingAdvanceSlider.setValue(getDisplayValue("timingAdvance"))
        if (startupPowerSlider) startupPowerSlider.setValue(getDisplayValue("startupPower"))
        if (motorKvSlider) motorKvSlider.setValue(getDisplayValue("motorKv"))
        if (motorPolesSlider) motorPolesSlider.setValue(getDisplayValue("motorPoles"))
        if (beeperVolumeSlider) beeperVolumeSlider.setValue(getDisplayValue("beepVolume"))
        if (pwmFreqSlider) pwmFreqSlider.setValue(getDisplayValue("pwmFrequency"))
        if (rampRateSlider) rampRateSlider.setValue(getDisplayValue("maxRampSpeed"))
        if (minDutyCycleSlider) minDutyCycleSlider.setValue(getDisplayValue("minDutyCycle"))
        if (tempLimitSlider) tempLimitSlider.setValue(getDisplayValue("temperatureLimit"))
        if (currentLimitSlider) currentLimitSlider.setValue(getDisplayValue("currentLimit"))
        if (lowVoltageThresholdSlider) lowVoltageThresholdSlider.setValue(getDisplayValue("lowVoltageThreshold"))
        if (absoluteVoltageCutoffSlider) absoluteVoltageCutoffSlider.setValue(getDisplayValue("absoluteVoltageCutoff"))
        if (currentPidPSlider) currentPidPSlider.setValue(getDisplayValue("currentPidP"))
        if (currentPidISlider) currentPidISlider.setValue(getDisplayValue("currentPidI"))
        if (currentPidDSlider) currentPidDSlider.setValue(getDisplayValue("currentPidD"))

        // Also update checkboxes
        updateCheckboxValues()
    }

    function updateCheckboxValues() {
        // Update checkbox values when data changes
        // These are handled through their bindings in the checked property
        // Force a re-evaluation of bindings

        // TODO: Jake: Why is this here?
    }

    function getEscBorderColor(index) {
        if (!escStatusModel || index >= escStatusModel.count) {
            return "transparent"
        }

        var escData = escStatusModel.get(index)
        if (!escData || !escData.am32Eeprom) {
            return qgcPal.colorGrey
        }

        var isSelected = selectedEscs.indexOf(index) >= 0

        // Grey for unselected ESCs
        if (!isSelected) {
            return qgcPal.colorGrey
        }

        // Yellow for selected ESCs with unsaved changes
        if (escData.am32Eeprom.hasUnsavedChanges) {
            return qgcPal.colorYellow
        }

        // For selected ESCs without unsaved changes
        if (!escData.am32Eeprom.dataLoaded) {
            return qgcPal.colorRed  // Red if data not loaded
        }

        if (index === selectedEscs[0]) {
            return qgcPal.colorGreen  // First selected is always green (reference)
        }

        // Check if this ESC's settings match the reference ESC
        if (referenceEsc && referenceEsc.am32Eeprom && escData.am32Eeprom.settingsMatch(referenceEsc.am32Eeprom)) {
            return qgcPal.colorGreen  // Green if matches reference
        } else {
            return qgcPal.colorRed  // Red if doesn't match reference
        }
    }

    function toggleEscSelection(index) {
        var idx = selectedEscs.indexOf(index)
        var newSelection = selectedEscs.slice()
        var escData = escStatusModel.get(index)

        if (idx >= 0) {
            // Deselect ESC - clear any unsaved changes
            newSelection.splice(idx, 1)
            if (escData && escData.am32Eeprom && escData.am32Eeprom.hasUnsavedChanges) {
                escData.am32Eeprom.discardChanges()
            }
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
                firstSelected.am32Eeprom.requestReadAll(vehicle)
            }

            // Reset pending values and update sliders when selection changes
            pendingValues = {}
            updateSliderValues()
        }
    }

    function updatePendingValue(factName, value) {
        // Only update the value if it's different
        var valueChanged = false;

        // Apply to all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom) {
                var valueChanged = value != escData.am32Eeprom.getFactValue(factName);
                if (valueChanged) {
                    var changes = {}
                    changes[factName] = value
                    console.info("updatePendingValue")
                    escData.am32Eeprom.applyPendingChanges(changes)
                }
            }
        }

        // Update the pending value if it's changed
        if (valueChanged) {
            var newPending = Object.assign({}, pendingValues)
            newPending[factName] = value
            pendingValues = newPending
        }
    }

    function getDisplayValue(factName) {
        // If we have a pending value, use that
        if (pendingValues.hasOwnProperty(factName)) {
            console.info("getDisplayValue --> we have a pending value: ", factName)
            return pendingValues[factName]
        }

        // Otherwise use the value from the reference ESC
        if (referenceFacts) {
            return referenceFacts.getFactValue(factName)
        }

        return null
    }

    function hasUnsavedChange(factName) {
        // Check if this specific setting has an unsaved change
        return pendingValues.hasOwnProperty(factName)
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
                escData.am32Eeprom.requestReadAll(vehicle)
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

        // Clear pending values and update UI
        pendingValues = {}
        updateSliderValues()
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

        // No ESCs available message
        Item {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            visible:            !escStatusModel || escStatusModel.count === 0

            QGCLabel {
                anchors.centerIn: parent
                text: qsTr("No AM32 ESCs detected. Please ensure your vehicle is connected and powered.")
                font.pointSize: ScreenTools.largeFontPointSize
            }
        }

        // ESC Selection Header
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: _margins / 2
            visible: escStatusModel && escStatusModel.count > 0

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

        // Loading indicator
        Item {
            Layout.fillWidth:   true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 10
            visible:            !initializationComplete && escStatusModel && escStatusModel.count > 0

            Column {
                anchors.centerIn: parent
                spacing: _margins

                QGCLabel {
                    text: qsTr("Loading ESC data...")
                    font.pointSize: ScreenTools.largeFontPointSize
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                QGCLabel {
                    text: loadedEscs.size + " / " + escStatusModel.count + qsTr(" ESCs loaded")
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                BusyIndicator {
                    running: true
                    anchors.horizontalCenter: parent.horizontalCenter
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
            visible:            initializationComplete && referenceFacts && referenceFacts.dataLoaded

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
                        QGCLabel {
                            text: qsTr("Protocol:") + (hasUnsavedChange("inputType") ? " *" : "")
                            color: hasUnsavedChange("inputType") ? qgcPal.colorOrange : qgcPal.text
                        }
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
                            text:   qsTr("Stuck rotor protection") + (hasUnsavedChange("stuckRotorProtection") ? " *" : "")
                            checked: getDisplayValue("stuckRotorProtection") === true
                            textColor: hasUnsavedChange("stuckRotorProtection") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("stuckRotorProtection", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Stall protection") + (hasUnsavedChange("antiStall") ? " *" : "")
                            checked: getDisplayValue("antiStall") === true
                            textColor: hasUnsavedChange("antiStall") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("antiStall", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Use hall sensors") + (hasUnsavedChange("hallSensors") ? " *" : "")
                            checked: getDisplayValue("hallSensors") === true
                            textColor: hasUnsavedChange("hallSensors") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("hallSensors", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("30ms interval telemetry") + (hasUnsavedChange("telemetry30ms") ? " *" : "")
                            checked: getDisplayValue("telemetry30ms") === true
                            textColor: hasUnsavedChange("telemetry30ms") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("telemetry30ms", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Variable PWM") + (hasUnsavedChange("variablePwmFreq") ? " *" : "")
                            checked: getDisplayValue("variablePwmFreq") === true
                            textColor: hasUnsavedChange("variablePwmFreq") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("variablePwmFreq", checked)
                        }
                        QGCCheckBox {
                            text:   qsTr("Complementary PWM") + (hasUnsavedChange("complementaryPwm") ? " *" : "")
                            checked: getDisplayValue("complementaryPwm") === true
                            textColor: hasUnsavedChange("complementaryPwm") ? qgcPal.colorOrange : qgcPal.text
                            onClicked: updatePendingValue("complementaryPwm", checked)
                        }
                        QGCCheckBox {
                            id: autoTimingCheckbox
                            text:   qsTr("Auto timing advance") + (hasUnsavedChange("autoTiming") ? " *" : "")
                            textColor: hasUnsavedChange("autoTiming") ? qgcPal.colorOrange : qgcPal.text
                            checked: getDisplayValue("autoTiming") === true
                            onClicked: updatePendingValue("autoTiming", checked)
                        }

                        // Sliders with inline labels
                        Column {
                            QGCLabel {
                                text: qsTr("Timing advance") + (hasUnsavedChange("timingAdvance") ? " *" : "")
                                color: hasUnsavedChange("timingAdvance") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: timingAdvanceSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 30
                                majorTickStepSize:  1
                                decimalPlaces:      1
                                enabled:            !autoTimingCheckbox.checked
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("timingAdvance", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Startup power") + (hasUnsavedChange("startupPower") ? " *" : "")
                                color: hasUnsavedChange("startupPower") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: startupPowerSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               50
                                to:                 150
                                majorTickStepSize:  10
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("startupPower", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Motor KV") + (hasUnsavedChange("motorKv") ? " *" : "")
                                color: hasUnsavedChange("motorKv") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: motorKvSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               20
                                to:                 10220
                                majorTickStepSize:  1000
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("motorKv", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Motor poles") + (hasUnsavedChange("motorPoles") ? " *" : "")
                                color: hasUnsavedChange("motorPoles") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: motorPolesSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2
                                to:                 36
                                majorTickStepSize:  2
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("motorPoles", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Beeper volume") + (hasUnsavedChange("beepVolume") ? " *" : "")
                                color: hasUnsavedChange("beepVolume") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: beeperVolumeSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 11
                                majorTickStepSize:  1
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("beepVolume", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("PWM Frequency") + (hasUnsavedChange("pwmFrequency") ? " *" : "")
                                color: hasUnsavedChange("pwmFrequency") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: pwmFreqSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               8
                                to:                 48
                                majorTickStepSize:  5
                                decimalPlaces:      0
                                enabled:            getDisplayValue("variablePwmFreq") !== true
                                // Value set by updateSliderValues() after data is loaded
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
                            text:   qsTr("Disable stick calibration") + (hasUnsavedChange("disableStickCalibration") ? " *" : "")
                            textColor: hasUnsavedChange("disableStickCalibration") ? qgcPal.colorOrange : qgcPal.text
                            checked: getDisplayValue("disableStickCalibration") === true
                            onClicked: updatePendingValue("disableStickCalibration", checked)
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Ramp rate") + (hasUnsavedChange("maxRampSpeed") ? " *" : "")
                                color: hasUnsavedChange("maxRampSpeed") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: rampRateSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.1
                                to:                 20
                                majorTickStepSize:  2
                                decimalPlaces:      1
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("maxRampSpeed", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Minimum duty cycle") + (hasUnsavedChange("minDutyCycle") ? " *" : "")
                                color: hasUnsavedChange("minDutyCycle") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: minDutyCycleSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 25
                                majorTickStepSize:  5
                                decimalPlaces:      1
                                // Value set by updateSliderValues() after data is loaded
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
                            text:   qsTr("Low voltage cut off") + (hasUnsavedChange("lowVoltageCutoff") ? " *" : "")
                            textColor: hasUnsavedChange("lowVoltageCutoff") ? qgcPal.colorOrange : qgcPal.text
                            checked: getDisplayValue("lowVoltageCutoff") === true
                            onClicked: updatePendingValue("lowVoltageCutoff", checked)
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Temperature limit") + (hasUnsavedChange("temperatureLimit") ? " *" : "")
                                color: hasUnsavedChange("temperatureLimit") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: tempLimitSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               70
                                to:                 255
                                majorTickStepSize:  10
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("temperatureLimit", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Current limit") + (hasUnsavedChange("currentLimit") ? " *" : "")
                                color: hasUnsavedChange("currentLimit") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: currentLimitSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 206 // TODO: we need to use the min/max from the fact itself
                                majorTickStepSize:  20
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("currentLimit", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Low voltage threshold") + (hasUnsavedChange("lowVoltageThreshold") ? " *" : "")
                                color: hasUnsavedChange("lowVoltageThreshold") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: lowVoltageThresholdSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               2.5
                                to:                 3.5
                                majorTickStepSize:  0.2
                                decimalPlaces:      1
                                enabled:            lowVoltageCutoffCheckbox.checked
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("lowVoltageThreshold", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Absolute voltage cutoff") + (hasUnsavedChange("absoluteVoltageCutoff") ? " *" : "")
                                color: hasUnsavedChange("absoluteVoltageCutoff") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: absoluteVoltageCutoffSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0.5
                                to:                 50.0
                                majorTickStepSize:  5
                                decimalPlaces:      1
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("absoluteVoltageCutoff", value)
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroupLayout {
                    heading:            qsTr("Current Control")
                    Layout.fillWidth:   true
                    opacity:            currentLimitSlider.value > 100 ? 0.3 : 1.0

                    Flow {
                        width: parent.width
                        spacing: _margins

                        Column {
                            QGCLabel {
                                text: qsTr("Current P") + (hasUnsavedChange("currentPidP") ? " *" : "")
                                color: hasUnsavedChange("currentPidP") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: currentPidPSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 510
                                majorTickStepSize:  50
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("currentPidP", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Current I") + (hasUnsavedChange("currentPidI") ? " *" : "")
                                color: hasUnsavedChange("currentPidI") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: currentPidISlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 255
                                majorTickStepSize:  25
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
                                onValueChanged:     updatePendingValue("currentPidI", value)
                            }
                        }

                        Column {
                            QGCLabel {
                                text: qsTr("Current D") + (hasUnsavedChange("currentPidD") ? " *" : "")
                                color: hasUnsavedChange("currentPidD") ? qgcPal.colorOrange : qgcPal.text
                            }
                            ValueSlider {
                                id: currentPidDSlider
                                width:              ScreenTools.defaultFontPixelWidth * 30
                                from:               0
                                to:                 2550
                                majorTickStepSize:  250
                                decimalPlaces:      0
                                // Value set by updateSliderValues() after data is loaded
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
