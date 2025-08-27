import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Rectangle {
    id:                 root
    color:              qgcPal.window
    implicitWidth:      mainColumn.width + (_margins * 2)
    implicitHeight:     mainColumn.height + (_margins * 2)

    property var vehicle:           globals.activeVehicle
    property var escStatusModel:    vehicle ? vehicle.escs : null
    property var selectedEscs:      []  // Array of selected ESC indices
    property var am32Facts:         selectedEscs.length > 0 && escStatusModel ? escStatusModel.get(selectedEscs[0]).am32Eeprom : null

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins:   ScreenTools.defaultFontPixelHeight / 2
    readonly property real _indicatorSize:  ScreenTools.defaultFontPixelHeight * 0.4

    Component.onCompleted: {
        if (escStatusModel && escStatusModel.count > 0) {
            // Auto-select first ESC
            selectedEscs = [0]
        }
    }

    function toggleEscSelection(index) {
        var idx = selectedEscs.indexOf(index)
        if (idx >= 0) {
            selectedEscs.splice(idx, 1)
        } else {
            selectedEscs.push(index)
        }
        selectedEscs = selectedEscs.slice() // Trigger binding update

        // Load data for first selected ESC if needed
        if (selectedEscs.length > 0 && !escStatusModel.get(selectedEscs[0]).am32Eeprom.dataLoaded) {
            escStatusModel.get(selectedEscs[0]).am32Eeprom.requestRead(vehicle)
        }
    }

    Column {
        id:                 mainColumn
        anchors.centerIn:   parent
        spacing:            _margins

        // ESC Selection Header
        Row {
            spacing: _margins

            Repeater {
                model: escStatusModel ? escStatusModel.count : 0

                Rectangle {
                    width:          ScreenTools.defaultFontPixelWidth * 15
                    height:         ScreenTools.defaultFontPixelHeight * 5
                    radius:         ScreenTools.defaultFontPixelHeight / 4
                    border.width:   2
                    border.color:   selectedEscs.indexOf(index) >= 0 ? qgcPal.brandingPurple : qgcPal.windowShade
                    color:          selectedEscs.indexOf(index) >= 0 ? Qt.darker(qgcPal.brandingPurple, 1.8) : qgcPal.windowShade

                    property var escData: escStatusModel.get(index)
                    property bool isLoading: false  // TODO: Add loading state

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toggleEscSelection(index)
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 4

                            // Status indicators
                            Rectangle {
                                width:      _indicatorSize
                                height:     _indicatorSize
                                radius:     _indicatorSize / 2
                                color:      escData.am32Eeprom && escData.am32Eeprom.directionReversed ? qgcPal.warningText : qgcPal.textFieldText
                            }

                            Rectangle {
                                width:      _indicatorSize
                                height:     _indicatorSize
                                radius:     _indicatorSize / 2
                                color:      escData.am32Eeprom && escData.am32Eeprom.bidirectionalMode ? qgcPal.brandingBlue : qgcPal.textFieldText
                            }
                        }

                        QGCLabel {
                            text:                   qsTr("Bootloader")
                            font.pointSize:         ScreenTools.smallFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            text:                   escData.am32Eeprom ? "v" + escData.am32Eeprom.bootloaderVersion.value : "---"
                            font.pointSize:         ScreenTools.smallFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            text:                   qsTr("Firmware")
                            font.pointSize:         ScreenTools.smallFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            text:                   escData.am32Eeprom ?
                                                   "v" + escData.am32Eeprom.firmwareMajor.value + "." + escData.am32Eeprom.firmwareMinor.value
                                                   : "---"
                            font.pointSize:         ScreenTools.smallFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }

        // Settings Panel
        Flickable {
            width:              ScreenTools.defaultFontPixelWidth * 100
            height:             ScreenTools.defaultFontPixelHeight * 40
            contentHeight:      settingsColumn.height
            clip:               true
            visible:            am32Facts && am32Facts.dataLoaded

            Column {
                id:         settingsColumn
                width:      parent.width
                spacing:    _margins

                // Essentials Group
                SettingsGroup {
                    title: qsTr("Essentials")
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
                SettingsGroup {
                    title: qsTr("Motor")

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins
                        width:          parent.width

                        // Row 1 - Switches
                        FactCheckBox {
                            text:               qsTr("Stuck rotor protection")
                            fact:               am32Facts.stuckRotorProtection
                            Layout.fillWidth:   true
                        }

                        FactCheckBox {
                            text:               qsTr("Stall protection")
                            fact:               am32Facts.antiStall
                            Layout.fillWidth:   true
                        }

                        FactCheckBox {
                            text:               qsTr("Use hall sensors")
                            fact:               am32Facts.hallSensors
                            Layout.fillWidth:   true
                        }

                        // Row 2 - More switches
                        FactCheckBox {
                            text:               qsTr("30ms interval telemetry")
                            fact:               am32Facts.telemetry30ms
                            Layout.fillWidth:   true
                        }

                        FactCheckBox {
                            text:               qsTr("Variable PWM")
                            fact:               am32Facts.variablePwmFreq
                            Layout.fillWidth:   true
                        }

                        FactCheckBox {
                            text:               qsTr("Complementary PWM")
                            fact:               am32Facts.complementaryPwm
                            Layout.fillWidth:   true
                        }

                        // Row 3 - Sliders
                        SliderSetting {
                            label:      qsTr("Timing advance")
                            fact:       am32Facts.timingAdvance
                            from:       0
                            to:         30
                            suffix:     "°"
                            enabled:    !am32Facts.autoTiming.value
                        }

                        SliderSetting {
                            label:      qsTr("Startup power")
                            fact:       am32Facts.startupPower
                            from:       50
                            to:         150
                            suffix:     "%"
                        }

                        SliderSetting {
                            label:      qsTr("Motor KV")
                            fact:       am32Facts.motorKv
                            from:       20
                            to:         10220
                            stepSize:   40
                            showValue:  true
                        }

                        // Row 4
                        SliderSetting {
                            label:      qsTr("Motor poles")
                            fact:       am32Facts.motorPoles
                            from:       2
                            to:         36
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Beeper volume")
                            fact:       am32Facts.beepVolume
                            from:       0
                            to:         11
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("PWM Frequency")
                            fact:       am32Facts.pwmFrequency
                            from:       8
                            to:         48
                            suffix:     am32Facts.variablePwmFreq.value ? "kHz - " + (am32Facts.pwmFrequency.value * 2) + "kHz" : "kHz"
                            enabled:    !am32Facts.variablePwmFreq.value
                        }
                    }
                }

                // Extended Settings Group
                SettingsGroup {
                    title: qsTr("Extended settings")

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins
                        width:          parent.width

                        FactCheckBox {
                            text:               qsTr("Disable stick calibration")
                            fact:               am32Facts.directionReversed  // TODO: Add this fact
                            Layout.fillWidth:   true
                            enabled:            false  // Not in current fact model
                        }

                        Item { Layout.fillWidth: true }  // Spacer
                        Item { Layout.fillWidth: true }  // Spacer

                        SliderSetting {
                            label:      qsTr("Ramp rate")
                            fact:       am32Facts.maxRampSpeed
                            from:       0.1
                            to:         20
                            stepSize:   0.1
                            suffix:     "% duty cycle per ms"
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Minimum duty cycle")
                            fact:       am32Facts.minDutyCycle
                            from:       0
                            to:         25
                            stepSize:   0.5
                            suffix:     "%"
                            showValue:  true
                        }
                    }
                }

                // Limits Group
                SettingsGroup {
                    title: qsTr("Limits")

                    Column {
                        spacing: _groupMargins

                        FactCheckBox {
                            text:   qsTr("Low voltage cut off")
                            fact:   am32Facts.lowVoltageCutoff
                        }

                        GridLayout {
                            columns:        3
                            columnSpacing:  _margins * 2
                            rowSpacing:     _groupMargins
                            width:          parent.width

                            SliderSetting {
                                label:          qsTr("Temperature limit")
                                fact:           am32Facts.temperatureLimit
                                from:           70
                                to:             141
                                disabledValue:  141
                                suffix:         "°C"
                                showValue:      true
                            }

                            SliderSetting {
                                label:          qsTr("Current limit")
                                fact:           am32Facts.currentLimit
                                from:           0
                                to:             202
                                stepSize:       2
                                disabledValue:  202
                                suffix:         "A"
                                showValue:      true
                            }

                            SliderSetting {
                                label:      qsTr("Low voltage threshold")
                                fact:       am32Facts.lowVoltageThreshold
                                from:       2.5
                                to:         3.5
                                stepSize:   0.1
                                suffix:     "V/cell"
                                showValue:  true
                                enabled:    am32Facts.lowVoltageCutoff.value
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroup {
                    title:      qsTr("Current control")
                    opacity:    am32Facts.currentLimit.value > 100 ? 0.3 : 1.0

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins
                        width:          parent.width

                        SliderSetting {
                            label:      qsTr("Current P")
                            fact:       am32Facts.currentPidP
                            from:       0
                            to:         255
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Current I")
                            fact:       am32Facts.currentPidI
                            from:       0
                            to:         255
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Current D")
                            fact:       am32Facts.currentPidD
                            from:       0
                            to:         255
                            showValue:  true
                        }
                    }
                }

                // Sinusoidal Startup Group
                SettingsGroup {
                    title: qsTr("Sinusoidal Startup")

                    Column {
                        spacing: _groupMargins

                        FactCheckBox {
                            text:   qsTr("Sinusoidal startup")
                            fact:   am32Facts.sineStartup
                        }

                        GridLayout {
                            columns:        2
                            columnSpacing:  _margins * 2
                            rowSpacing:     _groupMargins
                            width:          parent.width

                            SliderSetting {
                                label:      qsTr("Sine Mode Range")
                                fact:       am32Facts.sineModeRange
                                from:       5
                                to:         25
                                suffix:     "%"
                                showValue:  true
                                enabled:    am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                            }

                            SliderSetting {
                                label:      qsTr("Sine Mode Power")
                                fact:       am32Facts.sineModeStrength
                                from:       1
                                to:         10
                                showValue:  true
                                enabled:    am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                            }
                        }
                    }
                }

                // Brake Group
                SettingsGroup {
                    title: qsTr("Brake")

                    Column {
                        spacing: _groupMargins

                        Row {
                            spacing: _margins * 2

                            FactCheckBox {
                                text:   qsTr("Brake on stop")
                                fact:   am32Facts.brakeOnStop
                            }

                            FactCheckBox {
                                text:   qsTr("Car type reverse breaking")
                                fact:   am32Facts.rcCarReversing
                            }
                        }

                        GridLayout {
                            columns:        3
                            columnSpacing:  _margins * 2
                            rowSpacing:     _groupMargins
                            width:          parent.width

                            SliderSetting {
                                label:      qsTr("Brake strength")
                                fact:       am32Facts.dragBrakeStrength
                                from:       1
                                to:         10
                                showValue:  true
                                enabled:    am32Facts.brakeOnStop.value && !am32Facts.rcCarReversing.value
                            }

                            SliderSetting {
                                label:      qsTr("Running brake level")
                                fact:       am32Facts.runningBrakeAmount
                                from:       1
                                to:         10
                                showValue:  true
                                enabled:    !am32Facts.rcCarReversing.value
                            }
                        }
                    }
                }
            }
        }

        // Action Buttons
        Row {
            spacing:            _margins
            layoutDirection:    Qt.RightToLeft
            width:              parent.width
            visible:            am32Facts && am32Facts.dataLoaded

            QGCButton {
                text:       qsTr("Write Settings")
                enabled:    am32Facts && am32Facts.hasUnsavedChanges
                highlighted: am32Facts && am32Facts.hasUnsavedChanges
                onClicked:  {
                    for (var i = 0; i < selectedEscs.length; i++) {
                        escStatusModel.get(selectedEscs[i]).am32Eeprom.requestWrite(vehicle)
                    }
                }
            }

            QGCButton {
                text:       qsTr("Read Settings")
                onClicked:  {
                    for (var i = 0; i < selectedEscs.length; i++) {
                        escStatusModel.get(selectedEscs[i]).am32Eeprom.requestRead(vehicle)
                    }
                }
            }
        }
    }

    // Component for settings groups
    Component {
        id: settingsGroupComponent

        Rectangle {
            property alias title: titleLabel.text
            default property alias content: contentItem.children

            width:          parent.width
            height:         titleLabel.height + contentItem.height + (_groupMargins * 3)
            color:          qgcPal.windowShade
            radius:         ScreenTools.defaultFontPixelHeight / 4

            QGCLabel {
                id:                 titleLabel
                text:               title
                font.bold:          true
                anchors.left:       parent.left
                anchors.top:        parent.top
                anchors.margins:    _groupMargins
            }

            Item {
                id:                 contentItem
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        titleLabel.bottom
                anchors.margins:    _groupMargins
                height:             childrenRect.height
            }
        }
    }

    // SettingsGroup component - Fixed cyclic alias issue
    component SettingsGroup: Loader {
        property string title: ""
        default property alias content: contentStore.children

        Item {
            id: contentStore
            visible: false
        }

        sourceComponent: settingsGroupComponent
        onLoaded: {
            item.title = title
            for (var i = 0; i < contentStore.children.length; i++) {
                contentStore.children[i].parent = item.children[1]
            }
        }
    }

    // SliderSetting component
    component SliderSetting: Column {
        property alias label: labelText.text
        property alias fact: factSlider.fact
        property alias from: factSlider.from
        property alias to: factSlider.to
        property alias suffix: suffixLabel.text
        property real stepSize: 1
        property bool showValue: false
        property int disabledValue: -1

        Layout.fillWidth: true
        spacing: 2

        Row {
            width: parent.width

            QGCLabel {
                id:     labelText
                width:  parent.width - valueLabel.width - suffixLabel.width - 10
                elide:  Text.ElideRight
            }

            QGCLabel {
                id:         valueLabel
                text:       fact ? fact.valueString : ""
                visible:    showValue
            }

            QGCLabel {
                id:         suffixLabel
                visible:    text !== ""
            }
        }

        FactSlider {
            id:             factSlider
            width:          parent.width
            majorTickStepSize: stepSize > 0 ? stepSize : (to - from) / 10  // Calculate reasonable tick spacing
        }
    }
}
