import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id: root

    property var vehicle: globals.activeVehicle
    property var eeproms: vehicle ? vehicle.am32eeproms : null
    property var selectedEeproms: []  // Array of selected EEPROM indices

    // Reference ESC is always the first selected
    property var firstEeprom: selectedEeproms.length > 0 && eeproms ? eeproms.get(selectedEeproms[0]) : null

    readonly property real _margins: ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins: ScreenTools.defaultFontPixelHeight / 2


    // Slider configurations matching your exact layout
    readonly property var motorSliderConfigs: [
        {settingName: "timingAdvance", label: qsTr("Timing advance")},
        {settingName: "startupPower", label: qsTr("Startup power")},
        {settingName: "motorKv", label: qsTr("Motor KV")},
        {settingName: "motorPoles", label: qsTr("Motor poles")},
        {settingName: "beepVolume", label: qsTr("Beeper volume")},
        {settingName: "pwmFrequency", label: qsTr("PWM Frequency")}
    ]

    readonly property var extendedSliderConfigs: [
        {settingName: "maxRampSpeed", label: qsTr("Ramp rate")},
        {settingName: "minDutyCycle", label: qsTr("Minimum duty cycle")}
    ]

    readonly property var limitsSliderConfigs: [
        {settingName: "temperatureLimit", label: qsTr("Temperature limit")},
        {settingName: "currentLimit", label: qsTr("Current limit")},
        {settingName: "lowVoltageThreshold", label: qsTr("Low voltage threshold")},
        {settingName: "absoluteVoltageCutoff", label: qsTr("Absolute voltage cutoff")}
    ]

    readonly property var currentControlSliderConfigs: [
        {settingName: "currentPidP", label: qsTr("Current P")},
        {settingName: "currentPidI", label: qsTr("Current I")},
        {settingName: "currentPidD", label: qsTr("Current D")}
    ]

    Component.onCompleted: {
        console.log("Component.onCompleted, eeproms.requestReadAll")
        eeproms.requestReadAll(vehicle)

        eeproms.countChanged.connect(function() {
            if (eeproms && eeproms.count > 0) {
                // Default: select all ESCs
                var all = []
                for (var i = 0; i < eeproms.count; i++) {
                    all.push(i)
                }
                console.log("selecting all eeproms")
                selectedEeproms = all
            }
        })
    }

    function getEscBorderColor(index) {
        if (!eeproms || index >= eeproms.count) return "transparent"

        var esc = eeproms.get(index)
        if (!esc) return qgcPal.colorGrey

        var isSelected = selectedEeproms.indexOf(index) >= 0
        if (!isSelected) return qgcPal.colorGrey

        // Yellow for pending changes
        if (esc.hasUnsavedChanges) return qgcPal.colorYellow

        // Orange if data not loaded
        if (!esc.dataLoaded) return qgcPal.colorOrange

        // Green for reference or matching reference
        if (index === selectedEeproms[0]) return qgcPal.colorGreen

        if (firstEeprom && esc.settingsMatch(firstEeprom)) {
            return qgcPal.colorGreen
        }

        return qgcPal.colorRed
    }

    function toggleEscSelection(index) {
        var idx = selectedEeproms.indexOf(index)
        var newSelection = selectedEeproms.slice()

        if (idx >= 0) {
            newSelection.splice(idx, 1)
            // Clear pending changes when deselecting
            var esc = eeproms.get(index)
            if (esc && esc.hasUnsavedChanges) {
                esc.discardChanges()
            }
        } else {
            newSelection.push(index)
            newSelection.sort(function(a, b) { return a - b })

            // Request data if needed
            var esc = eeproms.get(index)
            if (esc && !esc.dataLoaded) {
                esc.requestReadAll(vehicle)
            }
        }

        console.log("Updated selected");
        selectedEeproms = newSelection
    }

    function updateSetting(settingName, value) {
        // Apply to all selected ESCs
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc) {
                var setting = esc.getSetting(settingName)
                if (setting) {
                    setting.setPendingValue(value)
                }
            }
        }
    }

    function getSettingValue(settingName) {
        if (firstEeprom) {
            var setting = firstEeprom.getSetting(settingName)
            return setting ? setting.fact.rawValue : null
        }
        return null
    }

    function getSettingFact(settingName) {
        if (firstEeprom) {
            var setting = firstEeprom.getSetting(settingName)
            return setting ? setting.fact : null
        }
        return null
    }

    function hasUnsavedChange(settingName) {
        // Check if any selected ESC has pending changes for this setting
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc) {
                var setting = esc.getSetting(settingName)
                if (setting && setting.hasPendingChanges) {
                    return true
                }
            }
        }
        return false
    }

    function hasAnyUnsavedChanges() {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc && esc.hasUnsavedChanges) {
                return true
            }
        }
        return false
    }

    function writeSettings() {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc && esc.hasUnsavedChanges) {
                esc.requestWrite(vehicle)
            }
        }
    }

    function readSettings() {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc) {
                esc.requestReadAll(vehicle)
            }
        }
    }

    function discardChanges() {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc) {
                esc.discardChanges()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: _margins
        spacing: _margins

        // No ESCs available message
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !eeproms || eeproms.count === 0

            QGCLabel {
                anchors.centerIn: parent
                text: qsTr("No AM32 ESCs detected. Please ensure your vehicle is connected and powered.")
                font.pointSize: ScreenTools.largeFontPointSize
            }
        }

        // ESC Selection Row
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: _margins / 2
            visible: eeproms && eeproms.count > 0

            Repeater {
                // model: eeproms ? eeproms.count : 0
                model: eeproms ? eeproms : null

                Rectangle {
                    width: ScreenTools.defaultFontPixelWidth * 12
                    height: ScreenTools.defaultFontPixelHeight * 4
                    radius: ScreenTools.defaultFontPixelHeight / 4
                    border.width: 3
                    border.color: getEscBorderColor(index)
                    color: qgcPal.window

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toggleEscSelection(index)
                        cursorShape: Qt.PointingHandCursor
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        QGCLabel {
                            text: qsTr("ESC %1").arg(index + 1)
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            text: {
                                if (object.firmwareMajor && object.firmwareMinor) {
                                    return "v" + object.firmwareMajor.rawValue + "." + object.firmwareMinor.rawValue
                                }
                                return "---"
                            }
                            font.pointSize: ScreenTools.smallFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }

        // Settings Panel - Your exact layout
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: settingsColumn.height
            contentWidth: width
            clip: true
            visible: firstEeprom && firstEeprom.dataLoaded

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: _margins

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
                            spacing: _groupMargins / 2

                            QGCCheckBox {
                                text: qsTr("Stuck rotor protection") + (hasUnsavedChange("stuckRotorProtection") ? " *" : "")
                                checked: getSettingValue("stuckRotorProtection") === true
                                textColor: hasUnsavedChange("stuckRotorProtection") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("stuckRotorProtection", checked)
                            }
                            QGCCheckBox {
                                text: qsTr("Stall protection") + (hasUnsavedChange("antiStall") ? " *" : "")
                                checked: getSettingValue("antiStall") === true
                                textColor: hasUnsavedChange("antiStall") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("antiStall", checked)
                            }
                            QGCCheckBox {
                                text: qsTr("Use hall sensors") + (hasUnsavedChange("hallSensors") ? " *" : "")
                                checked: getSettingValue("hallSensors") === true
                                textColor: hasUnsavedChange("hallSensors") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("hallSensors", checked)
                            }
                            QGCCheckBox {
                                text: qsTr("30ms interval telemetry") + (hasUnsavedChange("telemetry30ms") ? " *" : "")
                                checked: getSettingValue("telemetry30ms") === true
                                textColor: hasUnsavedChange("telemetry30ms") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("telemetry30ms", checked)
                            }
                            QGCCheckBox {
                                id: variablePwmCheckbox
                                text: qsTr("Variable PWM") + (hasUnsavedChange("variablePwmFreq") ? " *" : "")
                                checked: getSettingValue("variablePwmFreq") === true
                                textColor: hasUnsavedChange("variablePwmFreq") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("variablePwmFreq", checked)
                            }
                            QGCCheckBox {
                                text: qsTr("Complementary PWM") + (hasUnsavedChange("complementaryPwm") ? " *" : "")
                                checked: getSettingValue("complementaryPwm") === true
                                textColor: hasUnsavedChange("complementaryPwm") ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting("complementaryPwm", checked)
                            }
                            QGCCheckBox {
                                id: autoTimingCheckbox
                                text: qsTr("Auto timing advance") + (hasUnsavedChange("autoTiming") ? " *" : "")
                                textColor: hasUnsavedChange("autoTiming") ? qgcPal.colorOrange : qgcPal.text
                                checked: getSettingValue("autoTiming") === true
                                onClicked: updateSetting("autoTiming", checked)
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
                                    color: "transparent"
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    property var settingFact: getSettingFact(modelData.settingName)

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.settingName
                                        label: modelData.label + (hasUnsavedChange(modelData.settingName) ? " *" : "")
                                        from: settingFact ? settingFact.min : 0
                                        to: settingFact ? settingFact.max : 100
                                        stepSize: settingFact ? settingFact.increment : 1
                                        decimalPlaces: settingFact ? settingFact.decimalPlaces : 0
                                        snapToStep: true
                                        value: getSettingValue(modelData.settingName) || 0
                                        enabled: modelData.settingName !== "pwmFrequency" || variablePwmCheckbox.checked

                                        onValueChange: function(name, value) {
                                            updateSetting(name, value)
                                        }

                                        Connections {
                                            target: firstEeprom
                                            enabled: firstEeprom !== null
                                            function onDataLoadedChanged() {
                                                sliderColumn.value = getSettingValue(modelData.settingName) || 0
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

                    RowLayout {
                        width: parent.width
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins

                            QGCCheckBox {
                                text: qsTr("Disable stick calibration") + (hasUnsavedChange("disableStickCalibration") ? " *" : "")
                                textColor: hasUnsavedChange("disableStickCalibration") ? qgcPal.colorOrange : qgcPal.text
                                checked: getSettingValue("disableStickCalibration") === true
                                onClicked: updateSetting("disableStickCalibration", checked)
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
                                    color: "transparent"
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    property var settingFact: getSettingFact(modelData.settingName)

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.settingName
                                        label: modelData.label + (hasUnsavedChange(modelData.settingName) ? " *" : "")
                                        from: settingFact ? settingFact.min : 0
                                        to: settingFact ? settingFact.max : 100
                                        stepSize: settingFact ? settingFact.increment : 1
                                        decimalPlaces: settingFact ? settingFact.decimalPlaces : 0
                                        snapToStep: true
                                        value: getSettingValue(modelData.settingName) || 0
                                        enabled: true

                                        onValueChange: function(name, value) {
                                            updateSetting(name, value)
                                        }

                                        Connections {
                                            target: firstEeprom
                                            enabled: firstEeprom !== null
                                            function onDataLoadedChanged() {
                                                sliderColumn.value = getSettingValue(modelData.settingName) || 0
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
                                text: qsTr("Low voltage cut off") + (hasUnsavedChange("lowVoltageCutoff") ? " *" : "")
                                textColor: hasUnsavedChange("lowVoltageCutoff") ? qgcPal.colorOrange : qgcPal.text
                                checked: getSettingValue("lowVoltageCutoff") === true
                                onClicked: updateSetting("lowVoltageCutoff", checked)
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
                                    color: "transparent"
                                    border.color: qgcPal.text
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    property var settingFact: getSettingFact(modelData.settingName)

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        factName: modelData.settingName
                                        label: modelData.label + (hasUnsavedChange(modelData.settingName) ? " *" : "")
                                        from: settingFact ? settingFact.min : 0
                                        to: settingFact ? settingFact.max : 100
                                        stepSize: settingFact ? settingFact.increment : 1
                                        decimalPlaces: settingFact ? settingFact.decimalPlaces : 0
                                        snapToStep: true
                                        value: getSettingValue(modelData.settingName) || 0
                                        enabled: modelData.settingName !== "lowVoltageThreshold" || lowVoltageCutoffCheckbox.checked

                                        onValueChange: function(name, value) {
                                            updateSetting(name, value)
                                        }

                                        Connections {
                                            target: firstEeprom
                                            enabled: firstEeprom !== null
                                            function onDataLoadedChanged() {
                                                sliderColumn.value = getSettingValue(modelData.settingName) || 0
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
                    heading: qsTr("Current Control")
                    Layout.fillWidth: true
                    opacity: {
                        var currentLimit = getSettingValue("currentLimit")
                        return (currentLimit && currentLimit > 100) ? 0.3 : 1.0
                    }

                    GridLayout {
                        width: parent.width
                        columns: 3
                        rowSpacing: _margins
                        columnSpacing: _margins * 1.5

                        Repeater {
                            model: currentControlSliderConfigs

                            Rectangle {
                                color: "transparent"
                                border.color: qgcPal.text
                                border.width: 1
                                radius: 4
                                implicitWidth: sliderColumn.width + _groupMargins * 2
                                implicitHeight: sliderColumn.height + _groupMargins * 2

                                property var settingFact: getSettingFact(modelData.settingName)

                                AM32SettingSlider {
                                    id: sliderColumn
                                    anchors.centerIn: parent
                                    factName: modelData.settingName
                                    label: modelData.label + (hasUnsavedChange(modelData.settingName) ? " *" : "")
                                    from: settingFact ? settingFact.min : 0
                                    to: settingFact ? settingFact.max : 100
                                    stepSize: settingFact ? settingFact.increment : 1
                                    decimalPlaces: settingFact ? settingFact.decimalPlaces : 0
                                    snapToStep: true
                                    value: getSettingValue(modelData.settingName) || 0
                                    enabled: true

                                    onValueChange: function(name, value) {
                                        updateSetting(name, value)
                                    }

                                    Connections {
                                        target: firstEeprom
                                        enabled: firstEeprom !== null
                                        function onDataLoadedChanged() {
                                            sliderColumn.value = getSettingValue(modelData.settingName) || 0
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Action Buttons
                RowLayout {
                    Layout.fillWidth: true
                    layoutDirection: Qt.RightToLeft
                    spacing: _margins

                    QGCButton {
                        text: qsTr("Write Settings")
                        enabled: hasAnyUnsavedChanges() && selectedEeproms.length > 0
                        highlighted: hasAnyUnsavedChanges()
                        onClicked: writeSettings()
                    }

                    QGCButton {
                        text: qsTr("Read Settings")
                        enabled: selectedEeproms.length > 0
                        onClicked: readSettings()
                    }

                    QGCButton {
                        text: qsTr("Discard Changes")
                        enabled: hasAnyUnsavedChanges()
                        onClicked: discardChanges()
                    }
                }
            }
        }
    }
}
