import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:             powerPage
    pageComponent:  powerPageComponent

    property var _controller: powerModulePresetController

    PowerModulePresetController {
        id: powerModulePresetController
    }

    Component {
        id: powerPageComponent

        ColumnLayout {
            id:         flowLayout
            spacing:    _margins

            property int _batteryCount: battParams.getBatteryCount()
            property string _restartRequired: qsTr("Requires vehicle reboot")

            function _buildBatteryModel() {
                let model = []
                for (let i = 0; i < _batteryCount; i++) {
                    let prefix = battParams.prefixForIndex(i)
                    let monitorFact = powerModulePresetController.getParameterFact(-1, prefix + "MONITOR", false)
                    let inUse = monitorFact && monitorFact.rawValue !== 0
                    model.push(battParams.labelForIndex(i) + (inUse ? qsTr(" (in use)") : qsTr(" (not used)")))
                }
                return model
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            APMBatteryParams {
                id:             battParams
                controller:     _controller
                batteryIndex:   batterySelector.selectedBatteryIndex
            }

            // Battery selector
            ColumnLayout {
                spacing: _margins / 2

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:       qsTr("Battery")
                        font.bold:  true
                    }

                    QGCComboBox {
                        id:                     batterySelector
                        sizeToContents:         true
                        model:                  _buildBatteryModel()

                        property int selectedBatteryIndex: 0

                        onActivated: (index) => { selectedBatteryIndex = index }
                        onModelChanged: Qt.callLater(function() { currentIndex = selectedBatteryIndex })
                    }
                }

                // Monitor-only view (monitor disabled or params not yet available after enabling)
                QGCGroupBox {
                    title:                  qsTr("Battery Monitor")
                    visible:                !battParams.monitorEnabled || !battParams.paramsAvailable

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel { text: qsTr("Battery monitor") }
                            FactComboBox {
                                fact:           battParams.battMonitor
                                indexModel:     false
                                sizeToContents: true
                            }
                        }

                        QGCLabel {
                            text:       _restartRequired
                            visible:    battParams.showReboot
                        }

                        QGCButton {
                            text:       qsTr("Reboot vehicle")
                            visible:    battParams.showReboot
                            onClicked:  powerModulePresetController.vehicle.rebootVehicle()
                        }
                    }
                }

                // Full settings view
                QGCGroupBox {
                    id:                     fullSettingsRect
                    visible:                battParams.monitorEnabled && battParams.paramsAvailable

                    Loader {
                        id:                 batterySettingsLoader
                        sourceComponent:    fullSettingsRect.visible ? powerSetupComponent : undefined

                        // Force reload when battery selection changes so calcSensor re-runs
                        property int _batteryIndex: batterySelector.selectedBatteryIndex
                        on_BatteryIndexChanged: {
                            if (fullSettingsRect.visible) {
                                sourceComponent = undefined
                                sourceComponent = powerSetupComponent
                            }
                        }

                        property Fact armVoltMin:       battParams.battArmVolt
                        property Fact battAmpPerVolt:   battParams.battAmpPerVolt
                        property Fact battAmpOffset:    battParams.battAmpOffset
                        property Fact battCapacity:     battParams.battCapacity
                        property Fact battMonitor:      battParams.battMonitor
                        property Fact battVoltMult:     battParams.battVoltMult
                        property bool hasVoltageSensor: battParams.hasVoltageSensor
                        property bool hasCurrentSensor: battParams.hasCurrentSensor
                        property int  batteryIndex:     batterySelector.selectedBatteryIndex
                    }
                }
            }

        } // ColumnLayout
    } // Component - powerPageComponent

    Component {
        id: powerSetupComponent

        Column {
            spacing: _margins

            property real _margins:         ScreenTools.defaultFontPixelHeight / 2
            property bool _showAdvanced:    sensorCombo.currentIndex === sensorModel.count - 1
            property bool _showVoltageCalib: hasVoltageSensor && _showAdvanced
            property bool _showCurrentCalib: hasCurrentSensor && (_showAdvanced || !hasVoltageSensor)
            property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 15
            property real _comboWidth:       ScreenTools.defaultFontPixelWidth * 30

            Component.onCompleted: {
                loadSensorPresets()
                calcSensor()
            }

            function loadSensorPresets() {
                let presets = _controller.powerModulePresets()
                for (let i = 0; i < presets.length; i++) {
                    let preset = presets[i]
                    sensorModel.insert(sensorModel.count - 1,
                        { text: preset.name, voltMult: preset.voltMult, ampPerVolt: preset.ampPerVolt, ampOffset: preset.ampOffset })
                }
            }

            function calcSensor() {
                if (!hasVoltageSensor) {
                    sensorCombo.currentIndex = sensorModel.count - 1
                    return
                }
                for (let i=0; i<sensorModel.count - 1; i++) {
                    if (Math.abs(sensorModel.get(i).voltMult - battVoltMult.value) < 0.001 &&
                            Math.abs(sensorModel.get(i).ampPerVolt - battAmpPerVolt.value) < 0.0001 &&
                            Math.abs(sensorModel.get(i).ampOffset - battAmpOffset.value) < 0.0001) {
                        sensorCombo.currentIndex = i
                        return
                    }
                }
                sensorCombo.currentIndex = sensorModel.count - 1
            }

            ListModel {
                id: sensorModel

                ListElement {
                    text:       qsTr("Custom")
                }
            }

            GridLayout {
                columns:        2
                rowSpacing:     _margins
                columnSpacing:  _margins

                QGCLabel { text: qsTr("Battery monitor") }

                FactComboBox {
                    id:                     monitorCombo
                    Layout.maximumWidth:     _comboWidth
                    fact:                    battMonitor
                    indexModel:              false
                    sizeToContents:          true
                }

                QGCLabel { text: qsTr("Battery capacity") }

                FactTextField {
                    Layout.preferredWidth:  _textFieldWidth
                    fact:                   battCapacity
                }

                QGCLabel { text: qsTr("Minimum arming voltage") }

                FactTextField {
                    Layout.preferredWidth:  _textFieldWidth
                    fact:                   armVoltMin
                }

                QGCLabel {
                    text:    qsTr("Power sensor")
                    visible: hasVoltageSensor
                }

                QGCComboBox {
                    id:                     sensorCombo
                    Layout.maximumWidth:     _comboWidth
                    sizeToContents:         true
                    model:                  sensorModel
                    textRole:               "text"
                    visible:                hasVoltageSensor

                    onActivated: (index) => {
                        if (index < sensorModel.count - 1) {
                            battVoltMult.value = sensorModel.get(index).voltMult
                            battAmpPerVolt.value = sensorModel.get(index).ampPerVolt
                            battAmpOffset.value = sensorModel.get(index).ampOffset
                        }
                    }
                }

                QGCLabel {
                    text:    qsTr("Voltage multiplier")
                    visible: _showVoltageCalib
                }

                RowLayout {
                    visible:    _showVoltageCalib
                    spacing:    _margins

                    FactTextField {
                        Layout.preferredWidth:  _textFieldWidth
                        fact:                   battVoltMult
                    }

                    QGCButton {
                        text:      qsTr("Calculate")
                        onClicked: calcVoltageMultiplierDlgFactory.open({ batteryIndex: batteryIndex, battVoltMultFact: battVoltMult })
                    }
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Adjust this if the reported voltage does not match an external voltmeter reading. Click Calculate to compute a corrected value from a known measurement.")
                    visible:            _showVoltageCalib
                }

                QGCLabel {
                    text:    qsTr("Amps per volt")
                    visible: _showCurrentCalib
                }

                RowLayout {
                    visible:    _showCurrentCalib
                    spacing:    _margins

                    FactTextField {
                        Layout.preferredWidth:  _textFieldWidth
                        fact:                   battAmpPerVolt
                    }

                    QGCButton {
                        text:      qsTr("Calculate")
                        onClicked: calcAmpsPerVoltDlgFactory.open({ batteryIndex: batteryIndex, battAmpPerVoltFact: battAmpPerVolt })
                    }
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Adjust this if the reported current does not match an external current meter reading. Click Calculate to compute a corrected value from a known measurement.")
                    visible:            _showCurrentCalib
                }

                QGCLabel {
                    text:    qsTr("Amps Offset")
                    visible: _showCurrentCalib
                }

                FactTextField {
                    Layout.preferredWidth:  _textFieldWidth
                    fact:                   battAmpOffset
                    visible:                _showCurrentCalib
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Set this to the sensor voltage reading when no current is flowing. Corrects for a non-zero current reading at idle.")
                    visible:            _showCurrentCalib
                }

            } // GridLayout
        } // Column
    } // Component - powerSetupComponent

    QGCPopupDialogFactory {
        id: calcVoltageMultiplierDlgFactory

        dialogComponent: calcVoltageMultiplierDlgComponent
    }

    Component {
        id: calcVoltageMultiplierDlgComponent

        QGCPopupDialog {
            title:      qsTr("Calculate Voltage Multiplier")
            buttons:    Dialog.Close

            property int  batteryIndex
            property Fact battVoltMultFact

            property FactGroup _batteryFactGroup: powerModulePresetController.vehicle.batteries.count > batteryIndex ? powerModulePresetController.vehicle.getFactGroup("battery" + batteryIndex) : null
            property Fact      _vehicleVoltage:   _batteryFactGroup ? _batteryFactGroup.voltage : null
            property bool      _hasTelemetry:     _vehicleVoltage && _vehicleVoltage.value !== 0

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    text:                   qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new adjusted voltage multiplier.")
                }

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    visible:                !_hasTelemetry
                    text:                   qsTr("Vehicle voltage telemetry is not available. Connect to a vehicle with a powered battery to enable automatic calculation.")
                    color:                  qgcPal.warningText
                }

                GridLayout {
                    id:         gridLayout
                    columns:    2

                    QGCLabel {
                        text: qsTr("Measured voltage")
                    }
                    QGCTextField {
                        id: measuredVoltage
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                    }

                    QGCLabel {
                        text:    qsTr("Vehicle voltage")
                        visible: _hasTelemetry
                    }
                    FactLabel {
                        fact:    _vehicleVoltage
                        visible: _hasTelemetry
                    }

                    QGCLabel { text: qsTr("Voltage multiplier") }
                    FactLabel { fact: battVoltMultFact }
                }

                QGCButton {
                    text:    qsTr("Calculate And Set")
                    enabled: _hasTelemetry

                    onClicked: {
                        let measuredVoltageValue = parseFloat(measuredVoltage.text)
                        if (measuredVoltageValue === 0 || isNaN(measuredVoltageValue)) {
                            return
                        }
                        let newVoltageMultiplier = (measuredVoltageValue * battVoltMultFact.value) / _vehicleVoltage.value
                        if (newVoltageMultiplier > 0) {
                            battVoltMultFact.value = newVoltageMultiplier
                        }
                    }
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: calcAmpsPerVoltDlgFactory

        dialogComponent: calcAmpsPerVoltDlgComponent
    }

    Component {
        id: calcAmpsPerVoltDlgComponent

        QGCPopupDialog {
            title:      qsTr("Calculate Amps per Volt")
            buttons:    Dialog.Close

            property int  batteryIndex
            property Fact battAmpPerVoltFact

            property FactGroup _batteryFactGroup: powerModulePresetController.vehicle.batteries.count > batteryIndex ? powerModulePresetController.vehicle.getFactGroup("battery" + batteryIndex) : null
            property Fact      _vehicleCurrent:   _batteryFactGroup ? _batteryFactGroup.current : null
            property bool      _hasTelemetry:     _vehicleCurrent && _vehicleCurrent.value !== 0

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    text:                   qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
                }

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    visible:                !_hasTelemetry
                    text:                   qsTr("Vehicle current telemetry is not available. Connect to a vehicle with a powered battery to enable automatic calculation.")
                    color:                  qgcPal.warningText
                }

                GridLayout {
                    id:         gridLayout
                    columns:    2

                    QGCLabel {
                        text: qsTr("Measured current")
                    }
                    QGCTextField {
                        id: measuredCurrent
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 15
                    }

                    QGCLabel {
                        text:    qsTr("Vehicle current")
                        visible: _hasTelemetry
                    }
                    FactLabel {
                        fact:    _vehicleCurrent
                        visible: _hasTelemetry
                    }

                    QGCLabel { text: qsTr("Amps per volt") }
                    FactLabel { fact: battAmpPerVoltFact }
                }

                QGCButton {
                    text:    qsTr("Calculate And Set")
                    enabled: _hasTelemetry

                    onClicked: {
                        let measuredCurrentValue = parseFloat(measuredCurrent.text)
                        if (measuredCurrentValue === 0 || isNaN(measuredCurrentValue)) {
                            return
                        }
                        let newAmpsPerVolt = (measuredCurrentValue * battAmpPerVoltFact.value) / _vehicleCurrent.value
                        if (newAmpsPerVolt !== 0) {
                            battAmpPerVoltFact.value = newAmpsPerVolt
                        }
                    }
                }
            }
        }
    }

} // SetupPage
