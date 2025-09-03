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
    property var selectedEeproms: []
    property var firstEeprom: selectedEeproms.length > 0 && eeproms ? eeproms.get(selectedEeproms[0]) : null

    readonly property real _margins: ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins: ScreenTools.defaultFontPixelHeight / 2


    // TODO: swap order of label and settingName
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
        selectedEeproms = selectAllEeproms()
    }

    Connections {
        target: eeproms
        onCountChanged: {
            selectedEeproms = selectAllEeproms()
        }
    }

    function selectAllEeproms() {
        var selected = []
        for (var i = 0; i < eeproms.count; i++) {
            selected.push(i)
        }
        return selected
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
        }

        console.log("Updated selected");
        selectedEeproms = newSelection
    }

    function hasUnsavedChange(settingName) {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc && esc.settings[settingName] &&
                esc.settings[settingName].hasPendingChanges) {
                return true
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

    function discardChanges() {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc) {
                esc.discardChanges()
            }
        }
    }

    function updateSetting(settingName, value) {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc && esc.settings[settingName]) {
                esc.settings[settingName].setPendingValue(value)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: _margins
        spacing: _margins / 2

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

        QGCLabel {
            id: escSelectionLabel
            Layout.alignment: Qt.AlignHCenter
            // anchors.horizontalCenter:
            text: qsTr("Select ESCS to configure")
            font.pointSize: ScreenTools.mediumFontPointSize
            font.italic: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: _margins

            // ESC Selection Row
            Row {
                id: escSelectionRow
                // Layout.alignment: Qt.AlignHCenter
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

                            // TODO: show bootloader version
                            // TODO: guard this whole thing on eeprom version
                        }
                    }
                }
            }

            // Action Buttons
            ColumnLayout {
                // id: buttonsColumn
                // anchors.verticalCenter: escSelectionRow.verticalCenter
                Layout.fillWidth: true
                layoutDirection: Qt.RightToLeft
                spacing: _margins / 2

                QGCButton {
                    text: qsTr("Write Settings")
                    enabled: hasAnyUnsavedChanges() && selectedEeproms.length > 0
                    highlighted: hasAnyUnsavedChanges()
                    onClicked: writeSettings()
                }

                QGCButton {
                    text: qsTr("Read Settings")
                    onClicked: eeproms.requestReadAll(vehicle);
                }
            }
        }

        // Settings Panel
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
                        spacing: _margins * 2

                        // Left column with checkboxes
                        ColumnLayout {
                            // id: columnLayout1
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins / 2

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.stuckRotorProtection : null
                                text: qsTr("Stuck rotor protection") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.antiStall : null
                                text: qsTr("Stall protection") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.hallSensors : null
                                text: qsTr("Use hall sensors") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.telemetry30ms : null
                                text: qsTr("30ms interval telemetry") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.variablePwmFreq : null
                                text: qsTr("Variable PWM") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.complementaryPwm : null
                                text: qsTr("Complementary PWM") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
                            }
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.autoTiming : null
                                text: qsTr("Auto timing advance") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
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
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.settingName] : null

                                        onValueChanged: updateSetting(setting.name, value)
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
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            // id: columnLayout2
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.disableStickCalibration : null
                                text: qsTr("Disable stick calibration") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
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
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.settingName] : null

                                        onValueChanged: updateSetting(setting.name, value)
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
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            // id: columnLayout3
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.lowVoltageCutoff : null
                                text: qsTr("Low voltage cut off") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : qgcPal.text
                                onClicked: updateSetting(setting.name, checked)
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
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.settingName] : null

                                        onValueChanged: updateSetting(setting.name, value)
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

                    GridLayout {
                        width: parent.width
                        columns: 3
                        rowSpacing: _margins
                        columnSpacing: _margins * 1.5

                        Repeater {
                            model: currentControlSliderConfigs

                            Rectangle {
                                color: "transparent"
                                // border.color: qgcPal.groupBorder
                                // border.width: 1
                                radius: 4
                                implicitWidth: sliderColumn.width + _groupMargins * 2
                                implicitHeight: sliderColumn.height + _groupMargins * 2

                                AM32SettingSlider {
                                    id: sliderColumn
                                    anchors.centerIn: parent
                                    label: modelData.label
                                    setting: firstEeprom ? firstEeprom.settings[modelData.settingName] : null

                                    onValueChanged: updateSetting(setting.name, value)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
