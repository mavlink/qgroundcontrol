/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:         rootQGCView
    viewPanel:  panel

    property Fact battAmpPerVolt:   controller.getParameterFact(-1, "BATT_AMP_PERVOLT")
    property Fact battCapacity:     controller.getParameterFact(-1, "BATT_CAPACITY")
    property Fact battCurrPin:      controller.getParameterFact(-1, "BATT_CURR_PIN")
    property Fact battMonitor:      controller.getParameterFact(-1, "BATT_MONITOR")
    property Fact battVoltMult:     controller.getParameterFact(-1, "BATT_VOLT_MULT")
    property Fact battVoltPin:      controller.getParameterFact(-1, "BATT_VOLT_PIN")

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property bool _showAdvanced:    sensorCombo.currentIndex == sensorModel.count - 1

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

    QGCPalette { id: palette; colorGroupEnabled: panel.enabled }

    FactPanelController { id: controller; factPanel: panel }

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
            text:       "3DR Iris"
            voltPin:    2
            currPin:    3
            voltMult:   12.02
            ampPerVolt: 17.0
        }

        ListElement {
            text:       qsTr("Other")
        }
    }

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
                        text:       "Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new voltage multiplier."
                    }

                    Grid {
                        columns: 2
                        spacing: ScreenTools.defaultFontPixelHeight / 2
                        verticalItemAlignment: Grid.AlignVCenter

                        QGCLabel {
                            text: "Measured voltage:"
                        }
                        QGCTextField { id: measuredVoltage }

                        QGCLabel { text: "Vehicle voltage:" }
                        QGCLabel { text: controller.vehicle.battery.voltage.valueString }

                        QGCLabel { text: "Voltage multiplier:" }
                        FactLabel { fact: battVoltMult }
                    }

                    QGCButton {
                        text: "Calculate"

                        onClicked:  {
                            var measuredVoltageValue = parseFloat(measuredVoltage.text)
                            if (measuredVoltageValue == 0) {
                                return
                            }
                            var newVoltageMultiplier = (measuredVoltageValue * battVoltMult.value) / controller.vehicle.battery.voltage.value
                            if (newVoltageMultiplier != 0) {
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
                        text:       "Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value."
                    }

                    Grid {
                        columns: 2
                        spacing: ScreenTools.defaultFontPixelHeight / 2
                        verticalItemAlignment: Grid.AlignVCenter

                        QGCLabel {
                            text: "Measured current:"
                        }
                        QGCTextField { id: measuredCurrent }

                        QGCLabel { text: "Vehicle current:" }
                        QGCLabel { text: controller.vehicle.battery.current.valueString }

                        QGCLabel { text: "Amps per volt:" }
                        FactLabel { fact: battAmpPerVolt }
                    }

                    QGCButton {
                        text: "Calculate"

                        onClicked:  {
                            var measuredCurrentValue = parseFloat(measuredCurrent.text)
                            if (measuredCurrentValue == 0) {
                                return
                            }
                            var newAmpsPerVolt = (measuredCurrentValue * battAmpPerVolt.value) / controller.vehicle.battery.current.value
                            if (newAmpsPerVolt != 0) {
                                battAmpPerVolt.value = newAmpsPerVolt
                            }
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - calcAmpsPerVoltDlgComponent

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:       parent
            clip:               true
            contentWidth:       capacityField.x + capacityField.width + _margins
            contentHeight:      (_showAdvanced ? ampPerVoltHelp.y + ampPerVoltHelp.height : sensorCombo.y + sensorCombo.height) + _margins

            QGCLabel {
                id:                 monitorLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   monitorCombo.baseline
                text:               qsTr("Battery monitor:")
            }

            FactComboBox {
                id:                 monitorCombo
                anchors.topMargin:  _margins
                anchors.top:        parent.top
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battMonitor
                indexModel:         false
            }

            QGCLabel {
                id:                 capacityLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   capacityField.baseline
                text:               qsTr("Battery capacity:")
            }

            FactTextField {
                id:                 capacityField
                anchors.leftMargin: _margins
                anchors.topMargin:  _margins / 2
                anchors.top:        monitorCombo.bottom
                anchors.left:       capacityLabel.right
                width:              ScreenTools.defaultFontPixelWidth * 25
                fact:               battCapacity
            }

            QGCLabel {
                id:                 sensorLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   sensorCombo.baseline
                text:               qsTr("Power sensor:")
            }

            QGCComboBox {
                id:                 sensorCombo
                anchors.topMargin:  _margins
                anchors.top:        capacityField.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                model:              sensorModel

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
                id:                 currPinLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   currPinCombo.baseline
                text:               qsTr("Current pin:")
                visible:            _showAdvanced
            }

            FactComboBox {
                id:                 currPinCombo
                anchors.topMargin:  _margins
                anchors.top:        sensorCombo.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battCurrPin
                indexModel:         false
                visible:            _showAdvanced
            }

            QGCLabel {
                id:                 voltPinLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   voltPinCombo.baseline
                text:              qsTr("Voltage pin:")
                visible:            _showAdvanced
            }

            FactComboBox {
                id:                 voltPinCombo
                anchors.topMargin:  _margins / 2
                anchors.top:        currPinCombo.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battVoltPin
                indexModel:         false
                visible:            _showAdvanced
            }

            QGCLabel {
                id:                 voltMultLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   voltMultField.baseline
                text:               qsTr("Voltage multiplier:")
                visible:            _showAdvanced
            }

            FactTextField {
                id:                 voltMultField
                anchors.topMargin:  _margins
                anchors.top:        voltPinCombo.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battVoltMult
                visible:            _showAdvanced
            }

            QGCButton {
                id:                 voltMultCalculateButton
                anchors.margins:    _margins
                anchors.left:       voltMultField.right
                anchors.baseline:   voltMultField.baseline
                text:               "Calculate"
                visible:            _showAdvanced
                onClicked:          showDialog(calcVoltageMultiplierDlgComponent, qsTr("Calculate Voltage Multiplier"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }

            QGCLabel {
                id:                 voltMultHelp
                anchors.left:       voltMultLabel.left
                anchors.right:      voltMultCalculateButton.right
                anchors.topMargin:  _margins / 2
                anchors.top:        voltMultField.bottom
                font.pointSize:     ScreenTools.smallFontPointSize
                wrapMode:           Text.WordWrap
                text:               "If the battery voltage reported by the vehicle is largely different than the voltage read externally using a voltmeter you can adjust the voltage multiplier value to correct this. " +
                                    "Click the Calculate button for help with calculating a new value."
                visible:            _showAdvanced
            }

            QGCLabel {
                id:                 ampPerVoltLabel
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.baseline:   ampPerVoltField.baseline
                text:               qsTr("Amps per volt:")
                visible:            _showAdvanced
            }

            FactTextField {
                id:                 ampPerVoltField
                anchors.topMargin:  _margins
                anchors.top:        voltMultHelp.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battAmpPerVolt
                visible:            _showAdvanced
            }

            QGCButton {
                id:                 ampPerVoltCalculateButton
                anchors.margins:    _margins
                anchors.left:       ampPerVoltField.right
                anchors.baseline:   ampPerVoltField.baseline
                text:               "Calculate"
                visible:            _showAdvanced
                onClicked:          showDialog(calcAmpsPerVoltDlgComponent, qsTr("Calculate Amps per Volt"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }

            QGCLabel {
                id:                 ampPerVoltHelp
                anchors.left:       ampPerVoltLabel.left
                anchors.right:      ampPerVoltCalculateButton.right
                anchors.topMargin:  _margins / 2
                anchors.top:        ampPerVoltField.bottom
                font.pointSize:     ScreenTools.smallFontPointSize
                wrapMode:           Text.WordWrap
                text:               "If the current draw reported by the vehicle is largely different than the current read externally using a current meter you can adjust the amps per volt value to correct this. " +
                                    "Click the Calculate button for help with calculating a new value."
                visible:            _showAdvanced
            }
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
