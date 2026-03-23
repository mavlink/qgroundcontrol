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

    property var _controller: controller

    FactPanelController {
        id: controller
    }

    Component {
        id: powerPageComponent

        ColumnLayout {
            id:         flowLayout
            spacing:    _margins

            property int _batteryCount: _getBatteryCount()
            property string _restartRequired: qsTr("Requires vehicle reboot")

            // Local copy of prefix logic to avoid circular dependency on battParams during init
            function _batteryPrefix(index) {
                if (index === 0) {
                    return "BATT_"
                }
                if (index <= 8) {
                    return "BATT" + (index + 1) + "_"
                }
                return "BATT" + String.fromCharCode(65 + index - 9) + "_"
            }

            function _getBatteryCount() {
                for (var i = 0; i < 16; i++) {
                    if (!controller.parameterExists(-1, _batteryPrefix(i) + "MONITOR")) {
                        return i
                    }
                }
                return 16
            }

            function _batteryIndexLabel(index) {
                if (index <= 8) {
                    return String(index + 1)
                }
                return String.fromCharCode(65 + index - 9)
            }

            function _buildBatteryModel() {
                var model = []
                for (var i = 0; i < _batteryCount; i++) {
                    var prefix = _batteryPrefix(i)
                    var monitor = controller.getParameterFact(-1, prefix + "MONITOR", false)
                    var inUse = monitor && monitor.rawValue !== 0
                    model.push(_batteryIndexLabel(i) + (inUse ? qsTr(" (in use)") : qsTr(" (not used)")))
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
                        Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 15
                        model:                  _buildBatteryModel()

                        property int selectedBatteryIndex: 0

                        onActivated: (index) => { selectedBatteryIndex = index }
                        onModelChanged: Qt.callLater(function() { currentIndex = selectedBatteryIndex })
                    }
                }

                // Monitor-only view (monitor disabled or params not yet available after enabling)
                QGCGroupBox {
                    Layout.fillWidth:       true
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
                            onClicked:  controller.vehicle.rebootVehicle()
                        }
                    }
                }

                // Full settings view
                QGCGroupBox {
                    id:                     fullSettingsRect
                    Layout.fillWidth:       true
                    title:                  qsTr("Battery Settings")
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
                        property Fact battCurrPin:      battParams.battCurrPin
                        property Fact battMonitor:      battParams.battMonitor
                        property Fact battVoltMult:     battParams.battVoltMult
                        property Fact battVoltPin:      battParams.battVoltPin
                        property FactGroup  _batteryFactGroup:  fullSettingsRect.visible ? controller.vehicle.getFactGroup("battery" + batterySelector.selectedBatteryIndex) : null
                        property Fact vehicleVoltage:   _batteryFactGroup ? _batteryFactGroup.voltage : null
                        property Fact vehicleCurrent:   _batteryFactGroup ? _batteryFactGroup.current : null
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
            property real _fieldWidth:      ScreenTools.defaultFontPixelWidth * 25

            Component.onCompleted: calcSensor()

            function calcSensor() {
                for (var i=0; i<sensorModel.count - 1; i++) {
                    if (sensorModel.get(i).voltPin === battVoltPin.value &&
                            sensorModel.get(i).currPin === battCurrPin.value &&
                            Math.abs(sensorModel.get(i).voltMult - battVoltMult.value) < 0.001 &&
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
                    text:       qsTr("Power Module 90A")
                    voltPin:    2
                    currPin:    3
                    voltMult:   10.1
                    ampPerVolt: 17.0
                    ampOffset:  0
                }

                ListElement {
                    text:       qsTr("Power Module HV")
                    voltPin:    2
                    currPin:    3
                    voltMult:   12.02
                    ampPerVolt: 39.877
                    ampOffset:  0
                }

                ListElement {
                    text:       qsTr("3DR Iris")
                    voltPin:    2
                    currPin:    3
                    voltMult:   12.02
                    ampPerVolt: 17.0
                    ampOffset:  0
                }

                ListElement {
                    text:       qsTr("Blue Robotics Power Sense Module")
                    voltPin:    2
                    currPin:    3
                    voltMult:   11.000
                    ampPerVolt: 37.8788
                    ampOffset:  0.330
                }

                ListElement {
                    text:       qsTr("Navigator w/ Blue Robotics Power Sense Module")
                    voltPin:    5
                    currPin:    4
                    voltMult:   11.000
                    ampPerVolt: 37.8788
                    ampOffset:  0.330
                }

                ListElement {
                    text:       qsTr("Other")
                }
            }


            GridLayout {
                columns:        2
                rowSpacing:     _margins
                columnSpacing:  _margins

                QGCLabel { text: qsTr("Battery monitor") }

                FactComboBox {
                    id:         monitorCombo
                    fact:       battMonitor
                    indexModel: false
                    sizeToContents: true
                }

                QGCLabel { text: qsTr("Battery capacity") }

                FactTextField {
                    width:  _fieldWidth
                    fact:   battCapacity
                }

                QGCLabel { text: qsTr("Minimum arming voltage") }

                FactTextField {
                    width:  _fieldWidth
                    fact:   armVoltMin
                }

                QGCLabel { text: qsTr("Power sensor") }

                QGCComboBox {
                    id:                     sensorCombo
                    Layout.minimumWidth:    _fieldWidth
                    model:                  sensorModel
                    textRole:               "text"

                    onActivated: (index) => {
                        if (index < sensorModel.count - 1) {
                            battVoltPin.value = sensorModel.get(index).voltPin
                            battCurrPin.value = sensorModel.get(index).currPin
                            battVoltMult.value = sensorModel.get(index).voltMult
                            battAmpPerVolt.value = sensorModel.get(index).ampPerVolt
                            battAmpOffset.value = sensorModel.get(index).ampOffset
                        }
                    }
                }

                QGCLabel {
                    text:    qsTr("Current pin")
                    visible: _showAdvanced
                }

                FactComboBox {
                    Layout.minimumWidth: _fieldWidth
                    fact:                battCurrPin
                    indexModel:          false
                    visible:             _showAdvanced
                    sizeToContents:      true
                }

                QGCLabel {
                    text:    qsTr("Voltage pin")
                    visible: _showAdvanced
                }

                FactComboBox {
                    Layout.minimumWidth: _fieldWidth
                    fact:                battVoltPin
                    indexModel:          false
                    visible:             _showAdvanced
                    sizeToContents:      true
                }

                QGCLabel {
                    text:    qsTr("Voltage multiplier")
                    visible: _showAdvanced
                }

                RowLayout {
                    visible:    _showAdvanced
                    spacing:    _margins

                    FactTextField {
                        width:  _fieldWidth
                        fact:   battVoltMult
                    }

                    QGCButton {
                        text:      qsTr("Calculate")
                        onClicked: calcVoltageMultiplierDlgFactory.open({ vehicleVoltageFact: vehicleVoltage, battVoltMultFact: battVoltMult })
                    }
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Adjust this if the reported voltage does not match an external voltmeter reading. Click Calculate to compute a corrected value from a known measurement.")
                    visible:            _showAdvanced
                }

                QGCLabel {
                    text:    qsTr("Amps per volt")
                    visible: _showAdvanced
                }

                RowLayout {
                    visible:    _showAdvanced
                    spacing:    _margins

                    FactTextField {
                        width:  _fieldWidth
                        fact:   battAmpPerVolt
                    }

                    QGCButton {
                        text:      qsTr("Calculate")
                        onClicked: calcAmpsPerVoltDlgFactory.open({ vehicleCurrentFact: vehicleCurrent, battAmpPerVoltFact: battAmpPerVolt })
                    }
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Adjust this if the reported current does not match an external current meter reading. Click Calculate to compute a corrected value from a known measurement.")
                    visible:            _showAdvanced
                }

                QGCLabel {
                    text:    qsTr("Amps Offset")
                    visible: _showAdvanced
                }

                FactTextField {
                    width:      _fieldWidth
                    fact:       battAmpOffset
                    visible:    _showAdvanced
                }

                QGCLabel {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Set this to the sensor voltage reading when no current is flowing. Corrects for a non-zero current reading at idle.")
                    visible:            _showAdvanced
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

            property Fact vehicleVoltageFact
            property Fact battVoltMultFact

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    text:                   qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new adjusted voltage multiplier.")
                }

                GridLayout {
                    id:         gridLayout
                    columns:    2

                    QGCLabel {
                        text: qsTr("Measured voltage")
                    }
                    QGCTextField { id: measuredVoltage }

                    QGCLabel { text: qsTr("Vehicle voltage") }
                    FactLabel { fact: vehicleVoltageFact }

                    QGCLabel { text: qsTr("Voltage multiplier") }
                    FactLabel { fact: battVoltMultFact }
                }

                QGCButton {
                    text: qsTr("Calculate And Set")

                    onClicked:  {
                        var measuredVoltageValue = parseFloat(measuredVoltage.text)
                        if (measuredVoltageValue === 0 || isNaN(measuredVoltageValue) || !vehicleVoltageFact || !battVoltMultFact) {
                            return
                        }
                        var newVoltageMultiplier = (vehicleVoltageFact.value !== 0) ? (measuredVoltageValue * battVoltMultFact.value) / vehicleVoltageFact.value : 0
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

            property Fact vehicleCurrentFact
            property Fact battAmpPerVoltFact

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.preferredWidth:  gridLayout.width
                    wrapMode:               Text.WordWrap
                    text:                   qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
                }

                GridLayout {
                    id:         gridLayout
                    columns:    2

                    QGCLabel {
                        text: qsTr("Measured current")
                    }
                    QGCTextField { id: measuredCurrent }

                    QGCLabel { text: qsTr("Vehicle current") }
                    FactLabel { fact: vehicleCurrentFact }

                    QGCLabel { text: qsTr("Amps per volt") }
                    FactLabel { fact: battAmpPerVoltFact }
                }

                QGCButton {
                    text: qsTr("Calculate And Set")

                    onClicked:  {
                        var measuredCurrentValue = parseFloat(measuredCurrent.text)
                        if (measuredCurrentValue === 0 || isNaN(measuredCurrentValue) || !vehicleCurrentFact || !battAmpPerVoltFact) {
                            return
                        }
                        var newAmpsPerVolt = (vehicleCurrentFact.value !== 0) ? (measuredCurrentValue * battAmpPerVoltFact.value) / vehicleCurrentFact.value : 0
                        if (newAmpsPerVolt !== 0) {
                            battAmpPerVoltFact.value = newAmpsPerVolt
                        }
                    }
                }
            }
        }
    }

} // SetupPage
