import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:                 root
    anchors.fill:       parent

    property var vehicle:           globals.activeVehicle
    property var escStatusModel:    vehicle ? vehicle.escs : null
    property var selectedEscs:      []  // Array of selected ESC indices
    property var am32Facts:         selectedEscs.length > 0 && escStatusModel ? escStatusModel.get(selectedEscs[0]).am32Eeprom : null

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight
    readonly property real _groupMargins:   ScreenTools.defaultFontPixelHeight / 2
    readonly property real _indicatorSize:  ScreenTools.defaultFontPixelHeight * 0.4

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
        
        // First ESC is reference, check if other ESCs match
        if (index > 0 && escStatusModel.get(0) && escStatusModel.get(0).am32Eeprom) {
            var firstEsc = escStatusModel.get(0).am32Eeprom
            // Check a key parameter to see if settings match
            if (escData.am32Eeprom && escData.am32Eeprom.inputType && firstEsc.inputType &&
                (escData.am32Eeprom.inputType.value !== firstEsc.inputType.value ||
                 (escData.am32Eeprom.motorKv && firstEsc.motorKv && escData.am32Eeprom.motorKv.value !== firstEsc.motorKv.value))) {
                return qgcPal.warningText || "#FF4444"
            }
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

    Column {
        id:                 mainColumn
        anchors.fill:       parent
        anchors.margins:    _margins
        spacing:            _margins

        // ESC Selection Header
        Row {
            spacing: _margins / 2
            anchors.horizontalCenter: parent.horizontalCenter

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
            text:                   {
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
            anchors.horizontalCenter: parent.horizontalCenter
            font.italic:            true
            visible:                escStatusModel && escStatusModel.count > 0
        }

        // Status message when no ESC data available
        Column {
            anchors.centerIn: parent
            spacing: _margins
            visible: !am32Facts || !am32Facts.dataLoaded

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

        // Settings Panel
        Flickable {
            width:              parent.width
            height:             parent.height - y - actionButtons.height - (_margins * 2)
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
                            suffix:     " kHz"
                            enabled:    am32Facts.variablePwmFreq.value < 2
                        }
                    }
                }

                // Extended Settings Group
                SettingsGroup {
                    title: qsTr("Extended Settings")

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins
                        width:          parent.width

                        FactCheckBox {
                            text:               qsTr("Disable stick calibration")
                            fact:               am32Facts.disableStickCalibration
                            Layout.fillWidth:   true
                        }

                        Item { Layout.fillWidth: true }  // Spacer
                        Item { Layout.fillWidth: true }  // Spacer

                        SliderSetting {
                            label:      qsTr("Ramp rate")
                            fact:       am32Facts.maxRampSpeed
                            from:       0.1
                            to:         20
                            stepSize:   0.1
                            suffix:     "% duty/ms"
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
                                stepSize:   0.01
                                suffix:     "V/cell"
                                showValue:  true
                                enabled:    am32Facts.lowVoltageCutoff.value
                            }

                            SliderSetting {
                                label:      qsTr("Absolute voltage cutoff")
                                fact:       am32Facts.absoluteVoltageCutoff
                                from:       0.5
                                to:         50.0
                                stepSize:   0.5
                                suffix:     "V"
                                showValue:  true
                            }
                        }
                    }
                }

                // Current Control Group
                SettingsGroup {
                    title:      qsTr("Current Control")
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
                                label:      qsTr("Sine mode range")
                                fact:       am32Facts.sineModeRange
                                from:       5
                                to:         25
                                suffix:     "%"
                                showValue:  true
                                enabled:    am32Facts.sineStartup.value && !am32Facts.rcCarReversing.value
                            }

                            SliderSetting {
                                label:      qsTr("Sine mode power")
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
                                text:   qsTr("RC car reversing")
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

                // Servo Settings Group
                SettingsGroup {
                    title: qsTr("Servo Settings")

                    GridLayout {
                        columns:        3
                        columnSpacing:  _margins * 2
                        rowSpacing:     _groupMargins
                        width:          parent.width

                        SliderSetting {
                            label:      qsTr("Low threshold")
                            fact:       am32Facts.servoLowThreshold
                            from:       750
                            to:         1250
                            stepSize:   2
                            suffix:     "µs"
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("High threshold")
                            fact:       am32Facts.servoHighThreshold
                            from:       1750
                            to:         2250
                            stepSize:   2
                            suffix:     "µs"
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Neutral")
                            fact:       am32Facts.servoNeutral
                            from:       1374
                            to:         1630
                            stepSize:   1
                            suffix:     "µs"
                            showValue:  true
                        }

                        SliderSetting {
                            label:      qsTr("Dead band")
                            fact:       am32Facts.servoDeadband
                            from:       0
                            to:         100
                            suffix:     "µs"
                            showValue:  true
                        }
                    }
                }

            }
        }

        // Action Buttons
        Row {
            id:                 actionButtons
            spacing:            _margins
            layoutDirection:    Qt.RightToLeft
            width:              parent.width
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
                            // Copy settings from first selected ESC to this one
                            if (i > 0) {
                                // TODO: Copy facts from am32Facts to escData.am32Eeprom
                            }
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

    // SettingsGroup component
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
        visible: fact !== null && fact !== undefined

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
            majorTickStepSize: stepSize > 0 ? stepSize : (factSlider.to - factSlider.from) / 10
            visible:        fact !== null && fact !== undefined
        }
    }
}