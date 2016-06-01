/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 16
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 16

    property Fact _camTriggerMode:  controller.getParameterFact(-1, "TRIG_MODE")
    property Fact _camTriggerPol:   controller.getParameterFact(-1, "TRIG_POLARITY", false) // Don't bitch about missing as these only exist if trigger mode is enabled
    property Fact _auxPins:         controller.getParameterFact(-1, "TRIG_PINS",     false) // Ditto

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
                    //-- This will reboot the vehicle! We're set not to allow changes if armed.
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
            anchors.left:                               parent.left
            anchors.right:                              parent.right
            contentHeight:                              mainCol.height
            flickableDirection:                         Flickable.VerticalFlick
            Column {
                id:                                     mainCol
                spacing:                                _margins
                anchors.horizontalCenter:               parent.horizontalCenter
                /*
                   **** Camera Trigger ****
                */
                QGCLabel {
                    text:                               qsTr("Camera Trigger Settings")
                    font.family:                        ScreenTools.demiboldFontFamily
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
                            sourceSize.width:           width
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
                                    fact:               controller.getParameterFact(-1, "TRIG_INTERVAL", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _auxPins && _camTriggerMode.value === 2
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
                                    fact:               controller.getParameterFact(-1, "TRIG_DISTANCE", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _auxPins && _camTriggerMode.value === 3
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
                    font.family:                        ScreenTools.demiboldFontFamily
                    visible:                            _auxPins
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              camTrigRect.width
                    height:                             camHardwareRow.height + _margins * 2
                    visible:                            _auxPins
                    Row {
                        id:                             camHardwareRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
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
                                                color:  {
                                                    if(_auxPins) {
                                                        var pins = _auxPins.value.toString()
                                                        var pin  = (model.index + 1).toString()
                                                        if(pins.indexOf(pin) < 0)
                                                            return palette.windowShadeDark
                                                        else
                                                            return "green"
                                                    } else {
                                                        return palette.windowShade
                                                    }
                                                }
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        _auxChannels[model.index] = 1 - _auxChannels[model.index]
                                                        auxPin.color = _auxChannels[model.index] ? "green" : palette.windowShadeDark
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
                                        checked:        _camTriggerPol && _camTriggerPol.value === 0
                                        exclusiveGroup: polarityGroup
                                        text:           "Low (0V)"
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 0
                                            }
                                        }
                                    }
                                    QGCRadioButton {
                                        checked:        _camTriggerPol && _camTriggerPol.value > 0
                                        exclusiveGroup: polarityGroup
                                        text:           "High (3.3V)"
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 1
                                            }
                                        }
                                    }
                                }
                            }
                            Item { width: 1; height: _margins * 0.5; }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Trigger Period")
                                    width:              _middleRowWidth
                                    anchors.baseline:   trigPeriodField.baseline
                                    color:              palette.text
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
                }
            }
        }
    }
}

