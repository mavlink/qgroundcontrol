/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
        factPanel:  powerPage.viewPanel
    }

    Component {
        id: powerPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            property bool _batt2ParamsAvailable:    controller.parameterExists(-1, "BATT2_FS_LOW_ACT")
            property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
            property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            // Battery 1 settings
            Column {
                spacing: _margins / 2

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
                        sourceComponent:    powerSetupComponent

                        property Fact armVoltMin:       controller.getParameterFact(-1, "r.ARMING_VOLT_MIN")
                        property Fact battAmpPerVolt:   controller.getParameterFact(-1, "r.BATT_AMP_PERVLT")
                        property Fact battCapacity:     controller.getParameterFact(-1, "BATT_CAPACITY")
                        property Fact battCurrPin:      controller.getParameterFact(-1, "BATT_CURR_PIN")
                        property Fact battMonitor:      controller.getParameterFact(-1, "BATT_MONITOR")
                        property Fact battVoltMult:     controller.getParameterFact(-1, "BATT_VOLT_MULT")
                        property Fact battVoltPin:      controller.getParameterFact(-1, "BATT_VOLT_PIN")
                        property Fact vehicleVoltage:   controller.vehicle.battery.voltage
                        property Fact vehicleCurrent:   controller.vehicle.battery.current
                    }
                }
            }

            // Batter2 Monitor settings only - used when only monitor param is available
            Column {
                spacing: _margins / 2
                visible: _batt2MonitorAvailable && !_batt2ParamsAvailable

                QGCLabel {
                    text:       qsTr("Battery 2")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  batt2MonitorRow.x + batt2MonitorRow.width + _margins
                    height: batt2MonitorRow.y + batt2MonitorRow.height + _margins
                    color:  ggcPal.windowShade

                    RowLayout {
                        id:                 batt2MonitorRow
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        spacing:            ScreenTools.defaultFontPixelWidth
                        visible:            _batt2MonitorAvailable && !_batt2ParamsAvailable

                        QGCLabel { text: qsTr("Battery2 monitor:") }
                        FactComboBox {
                            id:         monitorCombo
                            fact:       _batt2Monitor
                            indexModel: false
                        }
                    }
                }
            }

            // Battery 2 settings - Used when full params are available
            Column {
                spacing: _margins / 2
                visible: _batt2ParamsAvailable

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
                        sourceComponent:    _batt2ParamsAvailable ? powerSetupComponent : undefined

                        property Fact armVoltMin:       controller.getParameterFact(-1, "r.ARMING_VOLT2_MIN", false /* reportMissing */)
                        property Fact battAmpPerVolt:   controller.getParameterFact(-1, "r.BATT2_AMP_PERVLT", false /* reportMissing */)
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
                            Math.abs(sensorModel.get(i).ampPerVolt - battAmpPerVolt.value) < 0.0001) {
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
                }

                ListElement {
                    text:       qsTr("Power Module HV")
                    voltPin:    2
                    currPin:    3
                    voltMult:   12.02
                    ampPerVolt: 39.877
                }

                ListElement {
                    text:       qsTr("3DR Iris")
                    voltPin:    2
                    currPin:    3
                    voltMult:   12.02
                    ampPerVolt: 17.0
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
                    id:                     monitorCombo
                    Layout.minimumWidth:    _fieldWidth
                    fact:                   battMonitor
                    indexModel:             false
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

                    onActivated: {
                        if (index < sensorModel.count - 1) {
                            battVoltPin.value = sensorModel.get(index).voltPin
                            battCurrPin.value = sensorModel.get(index).currPin
                            battVoltMult.value = sensorModel.get(index).voltMult
                            battAmpPerVolt.value = sensorModel.get(index).ampPerVolt
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
                    onClicked:  showDialog(calcVoltageMultiplierDlgComponent, qsTr("Calculate Voltage Multiplier"), qgcView.showDialogDefaultWidth, StandardButton.Close)
                    visible:    _showAdvanced
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
                    onClicked:  showDialog(calcAmpsPerVoltDlgComponent, qsTr("Calculate Amps per Volt"), qgcView.showDialogDefaultWidth, StandardButton.Close)
                    visible:    _showAdvanced
                }

                QGCLabel {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("If the current draw reported by the vehicle is largely different than the current read externally using a current meter you can adjust the amps per volt value to correct this. Click the Calculate button for help with calculating a new value.")
                    visible:            _showAdvanced
                }
            } // GridLayout
        } // Column
    } // Component - powerSetupComponent

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
                        text:       qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new voltage multiplier.")
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
                        QGCLabel { text: vehicleVoltage.valueString }

                        QGCLabel { text: qsTr("Voltage multiplier:") }
                        FactLabel { fact: battVoltMult }
                    }

                    QGCButton {
                        text: "Calculate"

                        onClicked:  {
                            var measuredVoltageValue = parseFloat(measuredVoltage.text)
                            if (measuredVoltageValue == 0 || isNaN(measuredVoltageValue)) {
                                return
                            }
                            var newVoltageMultiplier = (measuredVoltageValue * battVoltMult.value) / vehicleVoltage.value
                            if (newVoltageMultiplier > 0) {
                                battVoltMult.value = newVoltageMultiplier
                            }
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - calcVoltageMultiplierDlgComponent

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
                        QGCLabel { text: vehicleCurrent.valueString }

                        QGCLabel { text: qsTr("Amps per volt:") }
                        FactLabel { fact: battAmpPerVolt }
                    }

                    QGCButton {
                        text: "Calculate"

                        onClicked:  {
                            var measuredCurrentValue = parseFloat(measuredCurrent.text)
                            if (measuredCurrentValue == 0) {
                                return
                            }
                            var newAmpsPerVolt = (measuredCurrentValue * battAmpPerVolt.value) / vehicleCurrent.value
                            if (newAmpsPerVolt != 0) {
                                battAmpPerVolt.value = newAmpsPerVolt
                            }
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - calcAmpsPerVoltDlgComponent
} // SetupPage
