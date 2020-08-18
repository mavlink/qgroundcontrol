/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Controls.Styles      1.4
import QtQuick.Layouts              1.2
import QtGraphicalEffects           1.0

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             cameraPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  Math.max(availableWidth, innerColumn.width)
            height: innerColumn.height

            FactPanelController { id: controller; }

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 25

            property Fact _camTriggerMode:      controller.getParameterFact(-1, "TRIG_MODE")
            property Fact _camTriggerInterface: controller.getParameterFact(-1, "TRIG_INTERFACE", false /* reportMissing */)
            property Fact _camTriggerPol:       controller.getParameterFact(-1, "TRIG_POLARITY", false /* reportMissing */)
            property Fact _auxPins:             controller.getParameterFact(-1, "TRIG_PINS", false /* reportMissing */)

            property bool _rebooting:       false
            property var  _auxChannels:     [ 0, 0, 0, 0, 0, 0]

            function clearAuxArray() {
                for(var i = 0; i < 6; i++) {
                    _auxChannels[i] = 0
                }
            }

            function setAuxPins() {
                if(_auxPins) {
                    var values = ""
                    for(var i = 0; i < 6; i++) {
                        if(_auxChannels[i]) {
                            values += ((i+1).toString())
                        }
                    }
                    _auxPins.value = parseInt(values)
                }
            }

            Component.onCompleted: {
                if(_auxPins) {
                    clearAuxArray()
                    var values  = _auxPins.value.toString()
                    for(var i = 0; i < values.length; i++) {
                        var b = parseInt(values[i]) - 1
                        if(b >= 0 && b < 6) {
                            _auxChannels[b] = 1
                        }
                    }
                }
            }

            ColumnLayout {
                id:                         innerColumn
                anchors.horizontalCenter:   parent.horizontalCenter

                RowLayout {
                    id:         applyAndRestart
                    spacing:    _margins
                    visible:    false

                    QGCLabel {
                        text: qsTr("Vehicle must be restarted for changes to take effect.")
                    }
                    QGCButton {
                        text: qsTr("Apply and Restart")
                        onClicked:      {
                            //-- This will reboot the vehicle! We're set not to allow changes if armed.
                            QGroundControl.multiVehicleManager.activeVehicle.rebootVehicle()
                            applyAndRestart.visible = false
                            _rebooting = true
                        }
                    }
                }

                QGCGroupBox {
                    title:              qsTr("Camera Trigger Settings")
                    Layout.fillWidth:   true

                    GridLayout {
                        id:             cameraTrggerGrid
                        rows:           4
                        columns:        3
                        columnSpacing:  ScreenTools.defaultFontPixelWidth

                        QGCColoredImage {
                            id:                 triggerImage
                            color:              qgcPal.text
                            height:             ScreenTools.defaultFontPixelWidth * 10
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            sourceSize.width:   width
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            source:             "/qmlimages/CameraTrigger.svg"
                            Layout.rowSpan:     4
                        }

                        QGCLabel {
                            Layout.alignment:   Qt.AlignVCenter
                            text:               qsTr("Trigger mode")
                        }
                        FactComboBox {
                            fact:               _camTriggerMode
                            indexModel:         false
                            enabled:            !_rebooting
                            Layout.alignment:   Qt.AlignVCenter
                            Layout.minimumWidth: _editFieldWidth
                            onActivated: {
                                applyAndRestart.visible = true
                            }
                        }

                        QGCLabel {
                            Layout.alignment:   Qt.AlignVCenter
                            text:               qsTr("Trigger interface")
                        }
                        FactComboBox {
                            fact:               _camTriggerInterface
                            indexModel:         false
                            enabled:            !_rebooting && (_camTriggerInterface ? true : false)
                            Layout.alignment:   Qt.AlignVCenter
                            Layout.minimumWidth: _editFieldWidth
                            onActivated: {
                                applyAndRestart.visible = true
                            }
                        }

                        QGCLabel {
                            text:               qsTr("Time Interval")
                            Layout.alignment:   Qt.AlignVCenter
                            color:              qgcPal.text
                            visible:            timeIntervalField.visible
                        }
                        FactTextField {
                            id:                 timeIntervalField
                            fact:               controller.getParameterFact(-1, "TRIG_INTERVAL", false)
                            showUnits:          true
                            Layout.minimumWidth: _editFieldWidth
                            Layout.alignment:   Qt.AlignVCenter
                            visible:            _camTriggerMode.value === 2
                        }

                        QGCLabel {
                            text:               qsTr("Distance Interval")
                            Layout.alignment:   Qt.AlignVCenter
                            color:              qgcPal.text
                            visible:            trigDistField.visible
                        }
                        FactTextField {
                            id:                 trigDistField
                            fact:               controller.getParameterFact(-1, "TRIG_DISTANCE", false)
                            showUnits:          true
                            Layout.alignment:   Qt.AlignVCenter
                            Layout.minimumWidth: _editFieldWidth
                            visible:            _camTriggerMode.value === 3
                        }
                    }
                } // QGCGroupBox - Camera Trigger

                QGCGroupBox {
                    title:              qsTr("Hardware Settings")
                    visible:            _auxPins
                    Layout.fillWidth:   true

                    RowLayout {
                        spacing: _margins

                        // Aux pin assignment
                        ColumnLayout {
                            spacing: _margins

                            QGCLabel {
                                horizontalAlignment:    Text.AlignHCenter
                                text:                   qsTr("AUX Pin Assignment")
                                Layout.minimumWidth:    triggerImage.width
                            }

                            Row {
                                spacing:                _margins
                                Layout.alignment:       Qt.AlignHCenter

                                GridLayout {
                                    rows: 2
                                    columns: 6

                                    Repeater {
                                        model: _auxChannels

                                        QGCLabel {
                                            horizontalAlignment:    Text.AlignHCenter
                                            text:                   model.index + 1
                                        }
                                    }
                                    Repeater {
                                        model: _auxChannels

                                        Rectangle {
                                            id:             auxPin
                                            width:          ScreenTools.defaultFontPixelWidth * 2
                                            height:         ScreenTools.defaultFontPixelWidth * 2
                                            border.color:   qgcPal.text
                                            color:  {
                                                if(_auxPins) {
                                                    var pins = _auxPins.value.toString()
                                                    var pin  = (model.index + 1).toString()
                                                    if(pins.indexOf(pin) < 0)
                                                        return qgcPal.windowShadeDark
                                                    else
                                                        return "green"
                                                } else {
                                                    return qgcPal.windowShade
                                                }
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    _auxChannels[model.index] = 1 - _auxChannels[model.index]
                                                    auxPin.color = _auxChannels[model.index] ? "green" : qgcPal.windowShadeDark
                                                    setAuxPins()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } // ColumnLayout - Aux pins

                        // Trigger Pin Setup
                        ColumnLayout {
                            visible:    !_camTriggerInterface || (_camTriggerInterface.value === 1)
                            spacing:    _margins * 0.5

                            QGCLabel { text: qsTr("Trigger Pin Polarity") }

                            Row {
                                Item { height: 1; width: _margins; }
                                Column {
                                    spacing:            _margins * 0.5
                                    QGCRadioButton {
                                        checked:        _camTriggerPol && _camTriggerPol.value === 0
                                        text:           "Low (0V)"
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 0
                                            }
                                        }
                                    }
                                    QGCRadioButton {
                                        checked:        _camTriggerPol && _camTriggerPol.value > 0
                                        text:           "High (3.3V)"
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 1
                                            }
                                        }
                                    }
                                }
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    text:               qsTr("Trigger Period")
                                    anchors.baseline:   trigPeriodField.baseline
                                    color:              qgcPal.text
                                }
                                FactTextField {
                                    id:                 trigPeriodField
                                    fact:               controller.getParameterFact(-1, "TRIG_ACT_TIME", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                } // QGCGroupBox - Hardware Settings
            }
        }
    }
}
