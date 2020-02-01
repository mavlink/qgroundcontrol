/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             powerPage
    pageComponent:  powerPageComponent

    FactPanelController {
        id:         controller
    }

    Component {
        id: powerPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            property Fact _batt1Monitor:            controller.getParameterFact(-1, "BATT_MONITOR")
            property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
            property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
            property bool _batt1MonitorEnabled:     _batt1Monitor.rawValue !== 0
            property bool _batt2MonitorEnabled:     _batt2MonitorAvailable && _batt2Monitor.rawValue !== 0
            property bool _batt1ParamsAvailable:    controller.parameterExists(-1, "BATT_CAPACITY")
            property bool _batt2ParamsAvailable:    controller.parameterExists(-1, "BATT2_CAPACITY")
            property bool _showBatt1Reboot:         _batt1MonitorEnabled && !_batt1ParamsAvailable
            property bool _showBatt2Reboot:         _batt2MonitorEnabled && !_batt2ParamsAvailable
            property bool _escCalibrationAvailable: controller.parameterExists(-1, "ESC_CALIBRATION")
            property Fact _escCalibration:          controller.getParameterFact(-1, "ESC_CALIBRATION", false /* reportMissing */)

            property string _restartRequired: qsTr("Requires vehicle reboot")

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            // Battery1 Monitor settings only - used when only monitor param is available
            Column {
                spacing: _margins / 2
                visible: !_batt1MonitorEnabled || !_batt1ParamsAvailable

                QGCLabel {
                    text:       qsTr("Battery 1")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  batt1Column.x + batt1Column.width + _margins
                    height: batt1Column.y + batt1Column.height + _margins
                    color:  ggcPal.windowShade

                    ColumnLayout {
                        id:                 batt1Column
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        spacing:            ScreenTools.defaultFontPixelWidth

                        RowLayout {
                            id:                 batt1MonitorRow
                            spacing:            ScreenTools.defaultFontPixelWidth

                            QGCLabel { text: qsTr("Battery1 monitor:") }
                            FactComboBox {
                                id:         monitor1Combo
                                fact:       _batt1Monitor
                                indexModel: false
                            }
                        }

                        QGCLabel {
                            text:       _restartRequired
                            visible:    _showBatt1Reboot
                        }

                        QGCButton {
                            text:       qsTr("Reboot vehicle")
                            visible:    _showBatt1Reboot
                            onClicked:  controller.vehicle.rebootVehicle()
                        }
                    }
                }
            }

            // Battery 1 settings
            Column {
                id:         _batt1FullSettings
                spacing:    _margins / 2
                visible:    _batt1MonitorEnabled && _batt1ParamsAvailable

                QGCLabel {
                    text:       qsTr("Battery 1")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  battery1Loader.x + battery1Loader.width + _margins
                    height: battery1Loader.y + battery1Loader.height + _margins
                    color:  ggcPal.windowShade

                    Loader {
                        id:                 battery1Loader
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        sourceComponent:    _batt1FullSettings.visible ? powerSetupComponent : undefined

                        property Fact armVoltMin:       controller.getParameterFact(-1, "r.BATT_ARM_VOLT", false /* reportMissing */)
                        property Fact battAmpPerVolt:   controller.getParameterFact(-1, "r.BATT_AMP_PERVLT", false /* reportMissing */)
                        property Fact battAmpOffset:    controller.getParameterFact(-1, "BATT_AMP_OFFSET", false /* reportMissing */)
                        property Fact battCapacity:     controller.getParameterFact(-1, "BATT_CAPACITY", false /* reportMissing */)
                        property Fact battCurrPin:      controller.getParameterFact(-1, "BATT_CURR_PIN", false /* reportMissing */)
                        property Fact battMonitor:      controller.getParameterFact(-1, "BATT_MONITOR", false /* reportMissing */)
                        property Fact battVoltMult:     controller.getParameterFact(-1, "BATT_VOLT_MULT", false /* reportMissing */)
                        property Fact battVoltPin:      controller.getParameterFact(-1, "BATT_VOLT_PIN", false /* reportMissing */)
                        property Fact vehicleVoltage:   controller.vehicle.battery.voltage
                        property Fact vehicleCurrent:   controller.vehicle.battery.current
                    }
                }
            }

            // Battery2 Monitor settings only - used when only monitor param is available
            Column {
                spacing: _margins / 2
                visible: !_batt2MonitorEnabled || !_batt2ParamsAvailable

                QGCLabel {
                    text:       qsTr("Battery 2")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  batt2Column.x + batt2Column.width + _margins
                    height: batt2Column.y + batt2Column.height + _margins
                    color:  ggcPal.windowShade

                    ColumnLayout {
                        id:                 batt2Column
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        spacing:            ScreenTools.defaultFontPixelWidth

                        RowLayout {
                            id:                 batt2MonitorRow
                            spacing:            ScreenTools.defaultFontPixelWidth

                            QGCLabel { text: qsTr("Battery2 monitor:") }
                            FactComboBox {
                                id:         monitor2Combo
                                fact:       _batt2Monitor
                                indexModel: false
                            }
                        }

                        QGCLabel {
                            text:       _restartRequired
                            visible:    _showBatt2Reboot
                        }

                        QGCButton {
                            text:       qsTr("Reboot vehicle")
                            visible:    _showBatt2Reboot
                            onClicked:  controller.vehicle.rebootVehicle()
                        }
                    }
                }
            }

            // Battery 2 settings - Used when full params are available
            Column {
                id:         batt2FullSettings
                spacing:    _margins / 2
                visible:    _batt2MonitorEnabled && _batt2ParamsAvailable

                QGCLabel {
                    text:       qsTr("Battery 2")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  battery2Loader.x + battery2Loader.width + _margins
                    height: battery2Loader.y + battery2Loader.height + _margins
                    color:  ggcPal.windowShade

                    Loader {
                        id:                 battery2Loader
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        sourceComponent:    batt2FullSettings.visible ? powerSetupComponent : undefined

                        property Fact armVoltMin:       controller.getParameterFact(-1, "r.BATT2_ARM_VOLT", false /* reportMissing */)
                        property Fact battAmpPerVolt:   controller.getParameterFact(-1, "r.BATT2_AMP_PERVLT", false /* reportMissing */)
                        property Fact battAmpOffset:    controller.getParameterFact(-1, "BATT2_AMP_OFFSET", false /* reportMissing */)
                        property Fact battCapacity:     controller.getParameterFact(-1, "BATT2_CAPACITY", false /* reportMissing */)
                        property Fact battCurrPin:      controller.getParameterFact(-1, "BATT2_CURR_PIN", false /* reportMissing */)
                        property Fact battMonitor:      controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
                        property Fact battVoltMult:     controller.getParameterFact(-1, "BATT2_VOLT_MULT", false /* reportMissing */)
                        property Fact battVoltPin:      controller.getParameterFact(-1, "BATT2_VOLT_PIN", false /* reportMissing */)
                        property Fact vehicleVoltage:   controller.vehicle.battery2.voltage
                        property Fact vehicleCurrent:   controller.vehicle.battery2.current
                    }
                }
            }

            Column {
                spacing:    _margins / 2
                visible:    _escCalibrationAvailable

                QGCLabel {
                    text:       qsTr("ESC Calibration")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  escCalibrationHolder.x + escCalibrationHolder.width + _margins
                    height: escCalibrationHolder.y + escCalibrationHolder.height + _margins
                    color:  ggcPal.windowShade

                    Column {
                        id:         escCalibrationHolder
                        x:          _margins
                        y:          _margins
                        spacing:    _margins

                        Column {
                            spacing: _margins

                            QGCLabel {
                                text:   qsTr("WARNING: Remove props prior to calibration!")
                                color:  qgcPal.warningText
                            }

                            Row {
                                spacing: _margins

                                QGCButton {
                                    text: qsTr("Calibrate")
                                    enabled:    _escCalibration && _escCalibration.rawValue === 0
                                    onClicked:  if(_escCalibration) _escCalibration.rawValue = 3
                                }

                                Column {
                                    enabled: _escCalibration && _escCalibration.rawValue === 3
                                    QGCLabel { text:   _escCalibration ? (_escCalibration.rawValue === 3 ? qsTr("Now perform these steps:") : qsTr("Click Calibrate to start, then:")) : "" }
                                    QGCLabel { text:   qsTr("- Disconnect USB and battery so flight controller powers down") }
                                    QGCLabel { text:   qsTr("- Connect the battery") }
                                    QGCLabel { text:   qsTr("- The arming tone will be played (if the vehicle has a buzzer attached)") }
                                    QGCLabel { text:   qsTr("- If using a flight controller with a safety button press it until it displays solid red") }
                                    QGCLabel { text:   qsTr("- You will hear a musical tone then two beeps") }
                                    QGCLabel { text:   qsTr("- A few seconds later you should hear a number of beeps (one for each battery cell you’re using)") }
                                    QGCLabel { text:   qsTr("- And finally a single long beep indicating the end points have been set and the ESC is calibrated") }
                                    QGCLabel { text:   qsTr("- Disconnect the battery and power up again normally") }
                                }
                            }
                        }
                    }
                }
            }
        } // Flow
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
                    if (sensorModel.get(i).voltPin == battVoltPin.value &&
                            sensorModel.get(i).currPin == battCurrPin.value &&
                            Math.abs(sensorModel.get(i).voltMult - battVoltMult.value) < 0.001 &&
                            Math.abs(sensorModel.get(i).ampPerVolt - battAmpPerVolt.value) < 0.0001 &&
                            Math.abs(sensorModel.get(i).ampOffset - battAmpOffset.value) < 0.0001) {
                        sensorCombo.currentIndex = i
                        return
                    }
                }
                sensorCombo.currentIndex = sensorModel.count - 1
            }

            QGCPalette { id: palette; colorGroupEnabled: true }

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
                    text:       qsTr("Blue Robotics Power Sense Module R2")
                    voltPin:    2
                    currPin:    3
                    voltMult:   11.000
                    ampPerVolt: 37.8788
                    ampOffset:  0.330
                }

                ListElement {
                    text:       qsTr("Other")
                }
            }


            GridLayout {
                columns:        3
                rowSpacing:     _margins
                columnSpacing:  _margins

                QGCLabel { text: qsTr("Battery monitor:") }

                FactComboBox {
                    id:         monitorCombo
                    fact:       battMonitor
                    indexModel: false
                }

                QGCLabel {
                    Layout.row:     1
                    Layout.column:  0
                    text:           qsTr("Battery capacity:")
                }

                FactTextField {
                    id:     capacityField
                    width:  _fieldWidth
                    fact:   battCapacity
                }

                QGCLabel {
                    Layout.row:     2
                    Layout.column:  0
                    text:           qsTr("Minimum arming voltage:")
                }

                FactTextField {
                    id:     armVoltField
                    width:  _fieldWidth
                    fact:   armVoltMin
                }

                QGCLabel {
                    Layout.row:     3
                    Layout.column:  0
                    text:           qsTr("Power sensor:")
                }

                QGCComboBox {
                    id:                     sensorCombo
                    Layout.minimumWidth:    _fieldWidth
                    model:                  sensorModel
                    textRole:               "text"

                    onActivated: {
                        if (index < sensorModel.count - 1) {
                            battVoltPin.value = sensorModel.get(index).voltPin
                            battCurrPin.value = sensorModel.get(index).currPin
                            battVoltMult.value = sensorModel.get(index).voltMult
                            battAmpPerVolt.value = sensorModel.get(index).ampPerVolt
                            battAmpOffset.value = sensorModel.get(index).ampOffset
                        } else {

                        }
                    }
                }

                QGCLabel {
                    Layout.row:     4
                    Layout.column:  0
                    text:           qsTr("Current pin:")
                    visible:        _showAdvanced
                }

                FactComboBox {
                    Layout.minimumWidth:    _fieldWidth
                    fact:                   battCurrPin
                    indexModel:             false
                    visible:                _showAdvanced
                }

                QGCLabel {
                    Layout.row:     5
                    Layout.column:  0
                    text:           qsTr("Voltage pin:")
                    visible:        _showAdvanced
                }

                FactComboBox {
                    Layout.minimumWidth:    _fieldWidth
                    fact:                   battVoltPin
                    indexModel:             false
                    visible:                _showAdvanced
                }

                QGCLabel {
                    Layout.row:     6
                    Layout.column:  0
                    text:           qsTr("Voltage multiplier:")
                    visible:        _showAdvanced
                }

                FactTextField {
                    width:      _fieldWidth
                    fact:       battVoltMult
                    visible:    _showAdvanced
                }

                QGCButton {
                    text:       qsTr("Calculate")
                    visible:    _showAdvanced

                    onClicked: {
                        _calcVoltageDlgVehicleVoltage = vehicleVoltage
                        _calcVoltageDlgBattVoltMultParam = battVoltMult
                        mainWindow.showComponentDialog(calcVoltageMultiplierDlgComponent, qsTr("Calculate Voltage Multiplier"), mainWindow.showDialogDefaultWidth, StandardButton.Close)
                    }

                }

                QGCLabel {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("If the battery voltage reported by the vehicle is largely different than the voltage read externally using a voltmeter you can adjust the voltage multiplier value to correct this. Click the Calculate button for help with calculating a new value.")
                    visible:            _showAdvanced
                }

                QGCLabel {
                    text:       qsTr("Amps per volt:")
                    visible:    _showAdvanced
                }

                FactTextField {
                    width:      _fieldWidth
                    fact:       battAmpPerVolt
                    visible:    _showAdvanced
                }

                QGCButton {
                    text:       qsTr("Calculate")
                    visible:    _showAdvanced

                    onClicked: {
                        _calcAmpsPerVoltDlgVehicleCurrent = vehicleCurrent
                        _calcAmpsPerVoltDlgBattAmpPerVoltParam = battAmpPerVolt
                        mainWindow.showComponentDialog(calcAmpsPerVoltDlgComponent, qsTr("Calculate Amps per Volt"), mainWindow.showDialogDefaultWidth, StandardButton.Close)
                    }
                }

                QGCLabel {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("If the current draw reported by the vehicle is largely different than the current read externally using a current meter you can adjust the amps per volt value to correct this. Click the Calculate button for help with calculating a new value.")
                    visible:            _showAdvanced
                }

                QGCLabel {
                    text:       qsTr("Amps Offset:")
                    visible:    _showAdvanced
                }

                FactTextField {
                    width:      _fieldWidth
                    fact:       battAmpOffset
                    visible:    _showAdvanced
                }

                QGCLabel {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("If the vehicle reports a high current read when there is little or no current going through it, adjust the Amps Offset. It should be equal to the voltage reported by the sensor when the current is zero.")
                    visible:            _showAdvanced
                }

            } // GridLayout
        } // Column
    } // Component - powerSetupComponent

    // Must be set prior to use of calcVoltageMultiplierDlgComponent
    property Fact _calcVoltageDlgVehicleVoltage
    property Fact _calcVoltageDlgBattVoltMultParam

    Component {
        id: calcVoltageMultiplierDlgComponent

        QGCViewDialog {
            id: calcVoltageMultiplierDlg

            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  column.height
                contentWidth:   column.width

                Column {
                    id:         column
                    width:      calcVoltageMultiplierDlg.width
                    spacing:    ScreenTools.defaultFontPixelHeight

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new adjusted voltage multiplier.")
                    }

                    Grid {
                        columns: 2
                        spacing: ScreenTools.defaultFontPixelHeight / 2
                        verticalItemAlignment: Grid.AlignVCenter

                        QGCLabel {
                            text: qsTr("Measured voltage:")
                        }
                        QGCTextField { id: measuredVoltage }

                        QGCLabel { text: qsTr("Vehicle voltage:") }
                        FactLabel { fact: _calcVoltageDlgVehicleVoltage }

                        QGCLabel { text: qsTr("Voltage multiplier:") }
                        FactLabel { fact: _calcVoltageDlgBattVoltMultParam }
                    }

                    QGCButton {
                        text: qsTr("Calculate And Set")

                        onClicked:  {
                            var measuredVoltageValue = parseFloat(measuredVoltage.text)
                            if (measuredVoltageValue == 0 || isNaN(measuredVoltageValue)) {
                                return
                            }
                            var newVoltageMultiplier = (measuredVoltageValue * _calcVoltageDlgBattVoltMultParam.value) / _calcVoltageDlgVehicleVoltage.value
                            if (newVoltageMultiplier > 0) {
                                _calcVoltageDlgBattVoltMultParam.value = newVoltageMultiplier
                            }
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - calcVoltageMultiplierDlgComponent

    // Must be set prior to use of calcAmpsPerVoltDlgComponent
    property Fact _calcAmpsPerVoltDlgVehicleCurrent
    property Fact _calcAmpsPerVoltDlgBattAmpPerVoltParam

    Component {
        id: calcAmpsPerVoltDlgComponent

        QGCViewDialog {
            id: calcAmpsPerVoltDlg

            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  column.height
                contentWidth:   column.width

                Column {
                    id:         column
                    width:      calcAmpsPerVoltDlg.width
                    spacing:    ScreenTools.defaultFontPixelHeight

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
                    }

                    Grid {
                        columns: 2
                        spacing: ScreenTools.defaultFontPixelHeight / 2
                        verticalItemAlignment: Grid.AlignVCenter

                        QGCLabel {
                            text: qsTr("Measured current:")
                        }
                        QGCTextField { id: measuredCurrent }

                        QGCLabel { text: qsTr("Vehicle current:") }
                        FactLabel { fact: _calcAmpsPerVoltDlgVehicleCurrent }

                        QGCLabel { text: qsTr("Amps per volt:") }
                        FactLabel { fact: _calcAmpsPerVoltDlgBattAmpPerVoltParam }
                    }

                    QGCButton {
                        text: qsTr("Calculate And Set")

                        onClicked:  {
                            var measuredCurrentValue = parseFloat(measuredCurrent.text)
                            if (measuredCurrentValue == 0) {
                                return
                            }
                            var newAmpsPerVolt = (measuredCurrentValue * _calcAmpsPerVoltDlgBattAmpPerVoltParam.value) / _calcAmpsPerVoltDlgVehicleCurrent.value
                            if (newAmpsPerVolt != 0) {
                                _calcAmpsPerVoltDlgBattAmpPerVoltParam.value = newAmpsPerVolt
                            }
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - calcAmpsPerVoltDlgComponent
} // SetupPage
