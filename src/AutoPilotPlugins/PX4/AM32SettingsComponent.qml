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

    // Slider configurations matching your exact layout
    readonly property var motorSliderConfigs: [
        { label: qsTr("Timing advance"), name: "timingAdvance" },
        { label: qsTr("Startup power"), name: "startupPower" },
        { label: qsTr("Motor KV"), name: "motorKv" },
        { label: qsTr("Motor poles"), name: "motorPoles" },
        { label: qsTr("Beeper volume"), name: "beepVolume" },
        { label: qsTr("PWM Frequency"), name: "pwmFrequency" },
    ]

    readonly property var extendedSliderConfigs: [
        { label: qsTr("Ramp rate"), name: "maxRampSpeed" },
        { label: qsTr("Minimum duty cycle"), name: "minDutyCycle" },
    ]

    readonly property var limitsSliderConfigs: [
        { label: qsTr("Temperature limit"), name: "temperatureLimit" },
        { label: qsTr("Current limit"), name: "currentLimit" },
        { label: qsTr("Low voltage threshold"), name: "lowVoltageThreshold" },
        { label: qsTr("Absolute voltage cutoff"), name: "absoluteVoltageCutoff" },
    ]

    readonly property var currentControlSliderConfigs: [
        { label: qsTr("Current P"), name: "currentPidP" },
        { label: qsTr("Current I"), name: "currentPidI" },
        { label: qsTr("Current D"), name: "currentPidD" },
    ]

    Component.onCompleted: {
        eeproms.requestReadAll(vehicle)
        selectAllEeproms()
    }

    Connections {
        target: eeproms
        onCountChanged: {
            selectAllEeproms()
        }
    }

    function selectAllEeproms() {
        var selected = []
        for (var i = 0; i < eeproms.count; i++) {
            selected.push(i)
        }
        selectedEeproms = selected
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

        selectedEeproms = newSelection
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
        eeproms.requestWriteAll(vehicle, selectedEeproms)
    }

    function updateSetting(name, value) {
        for (var i = 0; i < selectedEeproms.length; i++) {
            var esc = eeproms.get(selectedEeproms[i])
            if (esc && esc.settings[name]) {
                esc.settings[name].setPendingValue(value)
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
            text: qsTr("Select ESCs to configure")
            font.pointSize: ScreenTools.mediumFontPointSize
            font.italic: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: _margins

            // ESC Selection Row
            Row {
                id: escSelectionRow
                spacing: _margins / 2
                visible: eeproms && eeproms.count > 0

                Repeater {
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

                            Grid {
                                columns: 2
                                columnSpacing: ScreenTools.defaultFontPixelWidth
                                rowSpacing: 1
                                anchors.horizontalCenter: parent.horizontalCenter

                                QGCLabel { text: "FW:"; font.pointSize: ScreenTools.smallFontPointSize }
                                QGCLabel {
                                    text: (object.firmwareMajor && object.firmwareMinor) ?
                                          object.firmwareMajor.rawValue + "." + object.firmwareMinor.rawValue : "---"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                }

                                QGCLabel { text: "BL:"; font.pointSize: ScreenTools.smallFontPointSize }
                                QGCLabel {
                                    text: object.bootloaderVersion ? object.bootloaderVersion.rawValue : "---"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                }

                                QGCLabel { text: "EE:"; font.pointSize: ScreenTools.smallFontPointSize }
                                QGCLabel {
                                    text: object.eepromVersion ? object.eepromVersion.rawValue : "---"
                                    font.pointSize: ScreenTools.smallFontPointSize
                                }
                            }
                        }
                    }
                }
            }

            // Action Buttons
            ColumnLayout {
                Layout.fillWidth: true
                spacing: _margins / 2

                QGCButton {
                    text: qsTr("Read Settings")
                    onClicked: eeproms.requestReadAll(vehicle);
                }

                QGCButton {
                    text: qsTr("Write Settings")
                    enabled: hasAnyUnsavedChanges() && selectedEeproms.length > 0
                    highlighted: hasAnyUnsavedChanges()
                    onClicked: writeSettings()
                }
            }
        } // RowLayout

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
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins / 2

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.stuckRotorProtection : null
                                visible: setting && firstEeprom.isSettingAvailable("stuckRotorProtection")
                                text: qsTr("Stuck rotor protection") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.stallProtection : null
                                visible: setting && firstEeprom.isSettingAvailable("stallProtection")
                                text: qsTr("Stall protection") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.hallSensors : null
                                visible: setting && firstEeprom.isSettingAvailable("hallSensors")
                                text: qsTr("Use hall sensors") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.telemetry30ms : null
                                visible: setting && firstEeprom.isSettingAvailable("telemetry30ms")
                                text: qsTr("30ms interval telemetry") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.variablePwmFreq : null
                                visible: setting && firstEeprom.isSettingAvailable("variablePwmFreq")
                                text: qsTr("Variable PWM") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.complementaryPwm : null
                                visible: setting && firstEeprom.isSettingAvailable("complementaryPwm")
                                text: qsTr("Complementary PWM") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
                                onClicked: updateSetting(setting.name, checked)
                            }

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.autoTiming : null
                                visible: setting && firstEeprom.isSettingAvailable("autoTiming")
                                text: qsTr("Auto timing advance") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
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
                                    id: motorSliderRect
                                    property string settingName: modelData.name

                                    visible: firstEeprom && firstEeprom.isSettingAvailable(settingName)
                                    color: "transparent"
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    // ESC match indicator dots
                                    Row {
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 4
                                        spacing: 2
                                        visible: eeproms && eeproms.count > 1

                                        Repeater {
                                            model: eeproms ? eeproms : null

                                            Rectangle {
                                                property var esc: object
                                                property var setting: esc ? esc.settings[motorSliderRect.settingName] : null

                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: {
                                                    if (!esc || !esc.dataLoaded) return qgcPal.colorGrey
                                                    if (!setting) return qgcPal.colorGrey
                                                    return setting.matchesMajority ? qgcPal.colorGreen : qgcPal.colorRed
                                                }
                                            }
                                        }
                                    }

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.name] : null
                                        onValueChanged: updateSetting(setting.name, value)
                                    }
                                }
                            }
                        }
                    }
                } // Motor Group

                // Extended Settings Group
                SettingsGroupLayout {
                    heading: qsTr("Extended Settings")
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins
                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.disableStickCalibration : null
                                visible: setting && firstEeprom.isSettingAvailable("disableStickCalibration")
                                text: qsTr("Disable stick calibration") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
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
                                    id: extendedSliderRect
                                    property string settingName: modelData.name

                                    visible: firstEeprom && firstEeprom.isSettingAvailable(settingName)
                                    color: "transparent"
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    // ESC match indicator dots
                                    Row {
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 4
                                        spacing: 2
                                        visible: eeproms && eeproms.count > 1

                                        Repeater {
                                            model: eeproms ? eeproms : null

                                            Rectangle {
                                                property var esc: object
                                                property var setting: esc ? esc.settings[extendedSliderRect.settingName] : null

                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: {
                                                    if (!esc || !esc.dataLoaded) return qgcPal.colorGrey
                                                    if (!setting) return qgcPal.colorGrey
                                                    return setting.matchesMajority ? qgcPal.colorGreen : qgcPal.colorRed
                                                }
                                            }
                                        }
                                    }

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.name] : null
                                        onValueChanged: updateSetting(setting.name, value)
                                    }
                                }
                            }
                        }
                    }
                } // Extended Settings Group

                // Limits Group
                SettingsGroupLayout {
                    heading: qsTr("Limits")
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: _margins * 2

                        // Left column with checkbox
                        ColumnLayout {
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 25
                            spacing: _groupMargins

                            QGCCheckBox {
                                property var setting: firstEeprom ? firstEeprom.settings.lowVoltageCutoff : null
                                visible: setting && firstEeprom.isSettingAvailable("lowVoltageCutoff")
                                text: qsTr("Low voltage cut off") + (setting && setting.hasPendingChanges ? " *" : "")
                                checked: setting ? setting.fact.rawValue === true : false
                                textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
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
                                    id: limitsSliderRect
                                    property string settingName: modelData.name

                                    visible: firstEeprom && firstEeprom.isSettingAvailable(settingName)
                                    color: "transparent"
                                    border.color: qgcPal.groupBorder
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: sliderColumn.width + _groupMargins * 2
                                    implicitHeight: sliderColumn.height + _groupMargins * 2

                                    // ESC match indicator dots
                                    Row {
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 4
                                        spacing: 2
                                        visible: eeproms && eeproms.count > 1

                                        Repeater {
                                            model: eeproms ? eeproms : null

                                            Rectangle {
                                                property var esc: object
                                                property var setting: esc ? esc.settings[limitsSliderRect.settingName] : null

                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: {
                                                    if (!esc || !esc.dataLoaded) return qgcPal.colorGrey
                                                    if (!setting) return qgcPal.colorGrey
                                                    return setting.matchesMajority ? qgcPal.colorGreen : qgcPal.colorRed
                                                }
                                            }
                                        }
                                    }

                                    AM32SettingSlider {
                                        id: sliderColumn
                                        anchors.centerIn: parent
                                        label: modelData.label
                                        setting: firstEeprom ? firstEeprom.settings[modelData.name] : null
                                        onValueChanged: updateSetting(setting.name, value)
                                    }
                                }
                            }
                        }
                    }
                } // Limits Group

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
                                id: currentControlSliderRect
                                property string settingName: modelData.name

                                visible: firstEeprom && firstEeprom.isSettingAvailable(settingName)
                                color: "transparent"
                                border.color: qgcPal.groupBorder
                                border.width: 1
                                radius: 4
                                implicitWidth: sliderColumn.width + _groupMargins * 2
                                implicitHeight: sliderColumn.height + _groupMargins * 2

                                // ESC match indicator dots
                                Row {
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 4
                                    spacing: 2
                                    visible: eeproms && eeproms.count > 1

                                    Repeater {
                                        model: eeproms ? eeproms : null

                                        Rectangle {
                                            property var esc: object
                                            property var setting: esc ? esc.settings[currentControlSliderRect.settingName] : null

                                            width: 8
                                            height: 8
                                            radius: 4
                                            color: {
                                                if (!esc || !esc.dataLoaded) return qgcPal.colorGrey
                                                if (!setting) return qgcPal.colorGrey
                                                return setting.matchesMajority ? qgcPal.colorGreen : qgcPal.colorRed
                                            }
                                        }
                                    }
                                }

                                AM32SettingSlider {
                                    id: sliderColumn
                                    anchors.centerIn: parent
                                    label: modelData.label
                                    setting: firstEeprom ? firstEeprom.settings[modelData.name] : null
                                    onValueChanged: updateSetting(setting.name, value)
                                }
                            }
                        } // Repeater
                    } // GridLayout
                } // Current Control Group
            } // ColumnLayout
        } // Flickable
    } // ColumnLayout
} // Item
