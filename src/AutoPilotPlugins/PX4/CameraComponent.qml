/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick                      2.5
import QtQuick.Controls             1.2
import QtQuick.Controls.Styles      1.2
import QtQuick.Layouts              1.2
import QtGraphicalEffects           1.0

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0


QGCView {
    id:             _cameraView
    viewPanel:      panel
    anchors.fill:   parent

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 22
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 18

    property Fact _camTriggerMode:  controller.getParameterFact(-1, "TRIG_MODE")

    property bool _hasFacts:        false
    property bool _rebooting:       false
    property var  _auxChannels:     [ 0, 0, 0, 0, 0, 0]

    function clearAuxArray() {
        for(var i = 0; i < 6; i++) {
            _auxChannels[i] = 0
        }
    }

    function setAuxPins() {
        var values = []
        for(var i = 0; i < 6; i++) {
            if(_auxChannels[i]) {
                values.push((i+1).toString())
            }
        }
        var auxFact = controller.getParameterFact(-1, "TRIG_PINS")
        auxFact.value = parseInt(values)
        console.log(values)
    }

    Component.onCompleted: {
        _hasFacts = _camTriggerMode.value > 0
        if(_hasFacts) {
            clearAuxArray()
            var auxFact = controller.getParameterFact(-1, "TRIG_PINS")
            var values  = auxFact.value.toString()
            console.log(values)
            for(var i = 0; i < values.length; i++) {
                var b = parseInt(values[i]) - 1
                if(b >= 0 && b < 6) {
                    console.log(b)
                    _auxChannels[b] = 1
                }
            }
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Item {
            id:                     applyAndRestart
            visible:                false
            anchors.top:            parent.top
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 10
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 10
            height:                 applyButton.height
            QGCLabel {
                anchors.left:       parent.left
                text:               qsTr("Vehicle must be restarted for changes to take effect. ")
            }
            QGCButton {
                id:                 applyButton
                anchors.right:      parent.right
                text:               qsTr("Apply and Restart")
                onClicked:      {
                    QGroundControl.multiVehicleManager.activeVehicle.rebootVehicle()
                    applyAndRestart.visible = false
                    _rebooting = true
                }
            }
        }
        QGCFlickable {
            clip:                                       true
            anchors.top:                                applyAndRestart.visible ? applyAndRestart.bottom : parent.top
            anchors.bottom:                             parent.bottom
            anchors.horizontalCenter:                   parent.horizontalCenter
            width:                                      mainCol.width
            contentHeight:                              mainCol.height
            contentWidth:                               mainCol.width
            flickableDirection:                         Flickable.VerticalFlick
            Column {
                id:                                     mainCol
                spacing:                                _margins
                /*
                   **** Camera Trigger ****
                */
                QGCLabel {
                    text:                               qsTr("Camera Trigger Settings")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    id:                                 camTrigRect
                    color:                              palette.windowShade
                    width:                              camTrigRow.width  + _margins * 2
                    height:                             camTrigRow.height + _margins * 2
                    Row {
                        id:                             camTrigRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        QGCColoredImage {
                            color:                      palette.text
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     "/qmlimages/CameraTrigger.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                visible:                !controller.fixedWing
                                QGCLabel {
                                    anchors.baseline:   camTrigCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Trigger mode:")
                                }
                                FactComboBox {
                                    id:                 camTrigCombo
                                    width:              _editFieldWidth
                                    fact:               _camTriggerMode
                                    indexModel:         false
                                    enabled:            !_rebooting
                                    onActivated: {
                                        applyAndRestart.visible = true
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Time Interval")
                                    width:              _middleRowWidth
                                    anchors.baseline:   timeIntervalField.baseline
                                    color:              palette.text
                                }
                                FactTextField {
                                    id:                 timeIntervalField
                                    fact:               _hasFacts ? controller.getParameterFact(-1, "TRIG_INTERVAL") : null
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _hasFacts && _camTriggerMode.value === 2
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Distance Interval")
                                    width:              _middleRowWidth
                                    anchors.baseline:   trigDistField.baseline
                                    color:              palette.text
                                }
                                FactTextField {
                                    id:                 trigDistField
                                    fact:               _hasFacts ? controller.getParameterFact(-1, "TRIG_DISTANCE") : null
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _hasFacts && _camTriggerMode.value === 3
                                }
                            }
                        }
                    }
                }
                /*
                   **** Camera Hardware ****
                */
                Item { width: 1; height: _margins * 0.5; }
                QGCLabel {
                    text:                               qsTr("Hardware Settings")
                    font.weight:                        Font.DemiBold
                    visible:                            _hasFacts
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              camTrigRect.width
                    height:                             camHardwareRow.height + _margins * 2
                    visible:                            _hasFacts
                    Row {
                        id:                             camHardwareRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter

                        property Fact _camTriggerPol:   controller.getParameterFact(-1, "TRIG_POLARITY")

                        Item { width: _margins * 0.5; height: 1; }
                        Item {
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            Column {
                                spacing:                ScreenTools.defaultFontPixelHeight
                                anchors.centerIn:       parent
                                QGCLabel {
                                    text:               "AUX Pin Assignment"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Row {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    Repeater {
                                        model:          _auxChannels
                                        Column {
                                            spacing:            ScreenTools.defaultFontPixelWidth * 0.5
                                            QGCLabel {
                                                text:           model.index + 1
                                                color:          palette.text
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Rectangle {
                                                id:             auxPin
                                                width:          ScreenTools.defaultFontPixelWidth * 2
                                                height:         ScreenTools.defaultFontPixelWidth * 2
                                                border.color:   palette.text
                                                color:          _auxChannels[model.index] ? "green" : palette.windowShadeDark
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        _auxChannels[model.index] = 1 - _auxChannels[model.index]
                                                        auxPin.color = _auxChannels[model.index] ? "green" : palette.windowShadeDark
                                                        console.log(model.index + " " + _auxChannels[model.index])
                                                        setAuxPins()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            QGCLabel {
                                id:                     returnHomeLabel
                                text:                   "Trigger Pin Polarity:"
                            }
                            Row {
                                Item { height: 1; width: _margins; }
                                Column {
                                    spacing:            _margins * 0.5
                                    ExclusiveGroup { id: polarityGroup }
                                    QGCRadioButton {
                                        checked:        _hasFacts && camHardwareRow._camTriggerPol.value === 0
                                        exclusiveGroup: polarityGroup
                                        text:           "Low (0V)"
                                        onClicked:      _camTriggerPol.value = 0
                                    }
                                    QGCRadioButton {
                                        checked:        _hasFacts && camHardwareRow._camTriggerPol.value > 0
                                        exclusiveGroup: polarityGroup
                                        text:           "High (3.3V)"
                                        onClicked:      _camTriggerPol.value = 1
                                    }
                                }
                            }
                            Item { width: 1; height: _margins; }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Trigger Period")
                                    width:              _middleRowWidth
                                    anchors.baseline:   trigPeriodField.baseline
                                    color:              palette.text
                                }
                                FactTextField {
                                    id:                 trigPeriodField
                                    fact:               controller.getParameterFact(-1, "TRIG_ACT_TIME")
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

