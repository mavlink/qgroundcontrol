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
    property var loadedEscs:        []
    property bool allDataLoaded:    false
    property bool initializationComplete: false

    // Store slider references for updates
    property var sliderRefs: ({})

    // Slider configurations
    readonly property var motorSliderConfigs: [
        {factName: "timingAdvance", label: qsTr("Timing advance"), from: 0, to: 30, step: 1, decimals: 1, snap: false},
        {factName: "startupPower", label: qsTr("Startup power"), from: 50, to: 150, step: 10, decimals: 0, snap: true},
        {factName: "motorKv", label: qsTr("Motor KV"), from: 20, to: 10220, step: 40, decimals: 0, snap: true},
        {factName: "motorPoles", label: qsTr("Motor poles"), from: 2, to: 36, step: 2, decimals: 0, snap: true},
        {factName: "beepVolume", label: qsTr("Beeper volume"), from: 0, to: 11, step: 1, decimals: 0, snap: true},
        {factName: "pwmFrequency", label: qsTr("PWM Frequency"), from: 8, to: 48, step: 1, decimals: 0, snap: true, conditionalEnable: "variablePwmFreq"}
    ]

    readonly property var extendedSliderConfigs: [
        {factName: "maxRampSpeed", label: qsTr("Ramp rate"), from: 0.1, to: 20, step: 0.1, decimals: 1, snap: 1},
        {factName: "minDutyCycle", label: qsTr("Minimum duty cycle"), from: 0, to: 25, step: 0.5, decimals: 1, snap: 1}
    ]

    readonly property var limitsSliderConfigs: [
        {factName: "temperatureLimit", label: qsTr("Temperature limit"), from: 70, to: 255, step: 1, decimals: 0, snap: 1},
        {factName: "currentLimit", label: qsTr("Current limit"), from: 0, to: 206, step: 1, decimals: 0, snap: 1},
        {factName: "lowVoltageThreshold", label: qsTr("Low voltage threshold"), from: 2.5, to: 3.5, step: 0.1, decimals: 1, snap: 1, conditionalEnable: "lowVoltageCutoff"},
        {factName: "absoluteVoltageCutoff", label: qsTr("Absolute voltage cutoff"), from: 0.5, to: 50.0, step: 0.5, decimals: 1, snap: 1}
    ]

    readonly property var currentControlSliderConfigs: [
        {factName: "currentPidP", label: qsTr("Current P"), from: 0, to: 510, step: 2, decimals: 0, snap: 1},
        {factName: "currentPidI", label: qsTr("Current I"), from: 0, to: 255, step: 1, decimals: 0, snap: 1},
        {factName: "currentPidD", label: qsTr("Current D"), from: 0, to: 2550, step: 10, decimals: 0, snap: 1}
    ]

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

                // Add to loaded ESCs list if not already there
                if (loadedEscs.indexOf(escIndex) === -1) {
                    loadedEscs.push(escIndex)
                }

                // Check if all ESCs have loaded
                if (loadedEscs.length === escStatusModel.count) {
                    console.info("All ESC data loaded")
                    dataLoadTimeout.stop()
                    finalizeInitialization()
                }
            }
        })
    }

    function finalizeInitialization() {
        if (!initializationComplete) {
            console.info("Finalizing initialization with " + loadedEscs.length + "/" + escStatusModel.count + " ESCs loaded")
            allDataLoaded = loadedEscs.length === escStatusModel.count
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
        for (var key in sliderRefs) {
            if (sliderRefs[key]) {
                var value = getDisplayValue(key)
                if (value !== null) {
                    sliderRefs[key].setValue(value)
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
        // Get the original value from the reference ESC
        var originalValue = null
        if (referenceFacts) {
            originalValue = referenceFacts.getOriginalValue(factName)
        }

        console.info("factName: ", factName)
        console.info("value: ", originalValue, "-->", value)

        // Check if value equals the original (no changes needed)
        var isOriginalValue = false
        if (originalValue !== null) {
            var diff = Math.abs(value - originalValue)
            isOriginalValue = diff < 0.001  // Consider values equal if difference is less than 0.001
        }

        // Apply to all selected ESCs
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom) {
                if (isOriginalValue) {
                    // Clear any pending changes for this fact
                    console.info("clr pnd chng")
                    escData.am32Eeprom.clearPendingChange(factName)
                } else {
                    // Apply the change
                    var changes = {}
                    changes[factName] = value
                    console.info("updatePendingValue: " + factName + " = " + value)
                    escData.am32Eeprom.applyPendingChanges(changes)
                }
            }
        }

        // Update local pending values tracking
        if (isOriginalValue) {
            // Remove from pending values if it's back to original
            var newPending = Object.assign({}, pendingValues)
            delete newPending[factName]
            pendingValues = newPending
        } else {
            // Add to pending values
            var newPending = Object.assign({}, pendingValues)
            newPending[factName] = value
            pendingValues = newPending
        }
    }

    function getDisplayValue(factName) {
        // If we have a pending value, use that
        if (pendingValues.hasOwnProperty(factName)) {
            return pendingValues[factName]
        }

        // Otherwise use the value from the reference ESC
        if (referenceFacts) {
            return referenceFacts.getFactValue(factName)
        }

        return null
    }

    function hasUnsavedChange(factName) {
        // Check if this specific setting has an unsaved change on any selected ESC
        for (var i = 0; i < selectedEscs.length; i++) {
            var escData = escStatusModel.get(selectedEscs[i])
            if (escData && escData.am32Eeprom && escData.am32Eeprom.hasPendingChange(factName)) {
                return true
            }
        }
        return false
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
                    text: loadedEscs.length + " / " + escStatusModel.count + qsTr(" ESCs loaded")
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

                // Motor Group
                SettingsGroupLayout {
                    heading: qsTr("Motor")
                    Layout.fillWidth: true

                    RowLayout {
                        width: parent.width
                        spacing: _margins * 2

                        // Left column with checkboxes
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins / 2  // Reduced spacing between checkboxes

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
                                id: variablePwmCheckbox
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

                            Item { Layout.fillHeight: true } // Spacer
                        }

                        // Right side with 3x2 grid of sliders
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            rowSpacing: _margins
                            columnSpacing: _margins * 1.5

                            Repeater {
                                model: motorSliderConfigs

                                Rectangle {
                                    color: "transparent"  // No background color
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.factName
                                        label: modelData.label + (hasUnsavedChange(modelData.factName) ? " *" : "")
                                        from: modelData.from
                                        to: modelData.to
                                        stepSize: modelData.step
                                        decimalPlaces: modelData.decimals
                                        snapToStep: modelData.snap
                                        // enabled: /* your conditional logic */
                                        enabled: true
                                        onValueChange: updatePendingValue

                                        Component.onCompleted: {
                                            sliderRefs[factName] = this
                                            var initialValue = getDisplayValue(factName)
                                            if (initialValue !== null) {
                                                setValue(initialValue)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Extended Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Extended Settings")
                    Layout.fillWidth: true

                    // TODO: do we need to set the width here such that changes to RowLayout and GridLayout don't screw with
                    // the spacing? Do we just need to enforce spacing?

                    RowLayout {
                        width: parent.width
                        // TODO: do we need to set the width based on the child?
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins

                            QGCCheckBox {
                                text:   qsTr("Disable stick calibration") + (hasUnsavedChange("disableStickCalibration") ? " *" : "")
                                textColor: hasUnsavedChange("disableStickCalibration") ? qgcPal.colorOrange : qgcPal.text
                                checked: getDisplayValue("disableStickCalibration") === true
                                onClicked: updatePendingValue("disableStickCalibration", checked)
                            }

                            Item { Layout.fillHeight: true } // Spacer
                        }

                        // Right side with sliders
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            rowSpacing: _margins
                            columnSpacing: _margins * 1.5

                            Repeater {
                                model: extendedSliderConfigs

                                Rectangle {
                                    color: "transparent"  // No background color
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.factName
                                        label: modelData.label + (hasUnsavedChange(modelData.factName) ? " *" : "")
                                        from: modelData.from
                                        to: modelData.to
                                        stepSize: modelData.step
                                        decimalPlaces: modelData.decimals
                                        snapToStep: modelData.snap
                                        // enabled: /* your conditional logic */
                                        enabled: true
                                        onValueChange: updatePendingValue

                                        Component.onCompleted: {
                                            sliderRefs[factName] = this
                                            var initialValue = getDisplayValue(factName)
                                            if (initialValue !== null) {
                                                setValue(initialValue)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Limits Group
                SettingsGroupLayout {
                    heading: qsTr("Limits")
                    Layout.fillWidth: true

                    RowLayout {
                        width: parent.width
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins

                            QGCCheckBox {
                                id: lowVoltageCutoffCheckbox
                                text:   qsTr("Low voltage cut off") + (hasUnsavedChange("lowVoltageCutoff") ? " *" : "")
                                textColor: hasUnsavedChange("lowVoltageCutoff") ? qgcPal.colorOrange : qgcPal.text
                                checked: getDisplayValue("lowVoltageCutoff") === true
                                onClicked: updatePendingValue("lowVoltageCutoff", checked)
                            }

                            Item { Layout.fillHeight: true } // Spacer
                        }

                        // Right side with 2x2 grid of sliders
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: _margins
                            columnSpacing: _margins * 1.5

                            Repeater {
                                model: limitsSliderConfigs

                                Rectangle {
                                    color: "transparent"  // No background color
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.factName
                                        label: modelData.label + (hasUnsavedChange(modelData.factName) ? " *" : "")
                                        from: modelData.from
                                        to: modelData.to
                                        stepSize: modelData.step
                                        decimalPlaces: modelData.decimals
                                        snapToStep: modelData.snap
                                        // enabled: /* your conditional logic */
                                        enabled: true
                                        onValueChange: updatePendingValue

                                        Component.onCompleted: {
                                            sliderRefs[factName] = this
                                            var initialValue = getDisplayValue(factName)
                                            if (initialValue !== null) {
                                                setValue(initialValue)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroupLayout {
                    heading:            qsTr("Current Control")
                    Layout.fillWidth:   true
                    opacity:            {
                        var currentLimitSlider = sliderRefs["currentLimit"]
                        return (currentLimitSlider && currentLimitSlider.value > 100) ? 0.3 : 1.0
                    }

                    GridLayout {
                        width: parent.width
                        columns: 3
                        rowSpacing: _margins
                        columnSpacing: _margins * 1.5

                        Repeater {
                            model: currentControlSliderConfigs

                            Rectangle {
                                color: "transparent"  // No background color
                                border.color: qgcPal.text
                                border.width: 1
                                radius: 4
                                implicitWidth: sliderColumn.width + _groupMargins * 2
                                implicitHeight: sliderColumn.height + _groupMargins * 2

                                AM32SettingSlider {
                                    id: sliderColumn
                                    anchors.centerIn: parent
                                    factName: modelData.factName
                                    label: modelData.label + (hasUnsavedChange(modelData.factName) ? " *" : "")
                                    from: modelData.from
                                    to: modelData.to
                                    stepSize: modelData.step
                                    decimalPlaces: modelData.decimals
                                    snapToStep: modelData.snap
                                    // enabled: /* your conditional logic */
                                    enabled: true
                                    onValueChange: updatePendingValue

                                    Component.onCompleted: {
                                        sliderRefs[factName] = this
                                        var initialValue = getDisplayValue(factName)
                                        if (initialValue !== null) {
                                            setValue(initialValue)
                                        }
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
