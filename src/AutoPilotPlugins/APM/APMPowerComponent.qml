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

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:       parent
            clip:               true
            contentWidth:       capacityField.x + capacityField.width + _margins
            contentHeight:      (_showAdvanced ? ampPerVoltField.y + ampPerVoltField.height : sensorCombo.y + sensorCombo.height) + _margins

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
                anchors.topMargin:  _margins / 2
                anchors.top:        voltPinCombo.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battVoltMult
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
                anchors.topMargin:  _margins / 2
                anchors.top:        voltMultField.bottom
                anchors.left:       capacityField.left
                width:              capacityField.width
                fact:               battAmpPerVolt
                visible:            _showAdvanced
            }
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
