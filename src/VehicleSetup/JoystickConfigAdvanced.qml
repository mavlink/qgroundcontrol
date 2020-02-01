/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    width:                  grid.width  + (ScreenTools.defaultFontPixelWidth  * 2)
    height:                 grid.height + (ScreenTools.defaultFontPixelHeight * 2)
    //---------------------------------------------------------------------
    GridLayout {
        id:                 grid
        columns:            2
        columnSpacing:      ScreenTools.defaultFontPixelWidth
        rowSpacing:         ScreenTools.defaultFontPixelHeight
        anchors.centerIn:   parent
        //-------------------------------------------------------------
        //-------------------------------------------------------------
        QGCRadioButton {
            text:               qsTr("Full down stick is zero throttle")
            checked:            _activeJoystick ? _activeJoystick.throttleMode === 1 : false
            onClicked:          _activeJoystick.throttleMode = 1
            Layout.columnSpan:  2
        }
        QGCRadioButton {
            text:               qsTr("Center stick is zero throttle")
            checked:            _activeJoystick ? _activeJoystick.throttleMode === 0 : false
            onClicked:          _activeJoystick.throttleMode = 0
            Layout.columnSpan:  2
        }
        //-------------------------------------------------------------
        QGCLabel {
            text:               qsTr("Spring loaded throttle smoothing")
            visible:            _activeJoystick ? _activeJoystick.throttleMode === 0 : false
            Layout.alignment:   Qt.AlignVCenter
            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 36
        }
        QGCCheckBox {
            checked:            _activeJoystick ? _activeJoystick.accumulator : false
            visible:            _activeJoystick ? _activeJoystick.throttleMode === 0 : false
            onClicked:          _activeJoystick.accumulator = checked
        }
        //-------------------------------------------------------------
        QGCLabel {
            text:               qsTr("Allow negative Thrust")
            visible:            activeVehicle.supportsNegativeThrust
            Layout.alignment:   Qt.AlignVCenter
        }
        QGCCheckBox {
            visible:            activeVehicle.supportsNegativeThrust
            enabled:            _activeJoystick.negativeThrust = activeVehicle.supportsNegativeThrust
            checked:            _activeJoystick ? _activeJoystick.negativeThrust : false
            onClicked:          _activeJoystick.negativeThrust = checked
        }
        //---------------------------------------------------------------------
        QGCLabel {
            text:               qsTr("Exponential:")
        }
        Row {
            spacing:            ScreenTools.defaultFontPixelWidth
            QGCSlider {
                id:             expoSlider
                width:          ScreenTools.defaultFontPixelWidth * 20
                minimumValue:   0
                maximumValue:   0.75
                Component.onCompleted: value = -_activeJoystick.exponential
                onValueChanged: _activeJoystick.exponential = -value
             }
            QGCLabel {
                id:     expoSliderIndicator
                text:   expoSlider.value.toFixed(2)
            }
        }
        //-----------------------------------------------------------------
        //-- Enable Advanced Mode
        QGCLabel {
            text:               qsTr("Enable further advanced settings (careful!)")
            Layout.alignment:   Qt.AlignVCenter
            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 36
        }
        QGCCheckBox {
            id:         advancedSettings
            checked:    activeVehicle.joystickMode !== 0
            onClicked: {
                if (!checked) {
                    activeVehicle.joystickMode = 0
                }
            }
        }
        //---------------------------------------------------------------------
        //-- Enable Gimbal
        QGCLabel {
            text:               qsTr("Enable gimbal control (Experimental)")
            visible:            advancedSettings.checked
            Layout.alignment:   Qt.AlignVCenter
        }
        QGCCheckBox {
            id:                 enabledGimbal
            visible:            advancedSettings.checked
            enabled:            _activeJoystick
            onClicked:          _activeJoystick.gimbalEnabled = checked
            Component.onCompleted: {
                checked = _activeJoystick.gimbalEnabled
            }
            Connections {
                target: joystickManager
                onActiveJoystickChanged: {
                    if(_activeJoystick) {
                        enabledGimbal.checked = Qt.binding(function() { return _activeJoystick.gimbalEnabled })
                    }
                }
            }
        }
        //-----------------------------------------------------------------
        //-- Mode
        QGCLabel {
            Layout.alignment:   Qt.AlignVCenter
            text:               qsTr("Joystick mode:")
            visible:            advancedSettings.checked
        }
        QGCComboBox {
            enabled:            advancedSettings.checked
            currentIndex:       activeVehicle.joystickMode
            width:              ScreenTools.defaultFontPixelWidth * 20
            model:              activeVehicle.joystickModes
            onActivated:        activeVehicle.joystickMode = index
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        //-----------------------------------------------------------------
        //-- Axis Message Frequency
        QGCLabel {
            text:               qsTr("Axis frequency (Hz):")
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        QGCTextField {
            text:               _activeJoystick.axisFrequency
            enabled:            advancedSettings.checked
            validator:          DoubleValidator { bottom: 0.25; top: 50.0; }
            inputMethodHints:   Qt.ImhFormattedNumbersOnly
            Layout.alignment:   Qt.AlignVCenter
            onEditingFinished: {
                _activeJoystick.axisFrequency = parseFloat(text)
            }
            visible:            advancedSettings.checked
        }
        //-----------------------------------------------------------------
        //-- Button Repeat Frequency
        QGCLabel {
            text:               qsTr("Button repeat frequency (Hz):")
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        QGCTextField {
            text:               _activeJoystick.buttonFrequency
            enabled:            advancedSettings.checked
            validator:          DoubleValidator { bottom: 0.25; top: 50.0; }
            inputMethodHints:   Qt.ImhFormattedNumbersOnly
            Layout.alignment:   Qt.AlignVCenter
            onEditingFinished: {
                _activeJoystick.buttonFrequency = parseFloat(text)
            }
            visible:            advancedSettings.checked
        }
        //-----------------------------------------------------------------
        //-- Enable circle correction
        QGCLabel {
            text:               qsTr("Enable circle correction")
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        QGCCheckBox {
            checked:            activeVehicle.joystickMode !== 0
            enabled:            advancedSettings.checked
            Component.onCompleted: {
                checked = _activeJoystick.circleCorrection
            }
            onClicked: {
                _activeJoystick.circleCorrection = checked
            }
            visible:            advancedSettings.checked
        }
        //-----------------------------------------------------------------
        //-- Deadband
        QGCLabel {
            text:               qsTr("Deadbands")
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        QGCCheckBox {
            enabled:            advancedSettings.checked
            checked:            controller.deadbandToggle
            onClicked:          controller.deadbandToggle = checked
            Layout.alignment:   Qt.AlignVCenter
            visible:            advancedSettings.checked
        }
        QGCLabel{
            Layout.fillWidth:   true
            Layout.columnSpan:  2
            font.pointSize:     ScreenTools.smallFontPointSize
            wrapMode:           Text.WordWrap
            visible:            advancedSettings.checked
            text:   qsTr("Deadband can be set during the first ") +
                    qsTr("step of calibration by gently wiggling each axis. ") +
                    qsTr("Deadband can also be adjusted by clicking and ") +
                    qsTr("dragging vertically on the corresponding axis monitor.")
        }
    }
}


