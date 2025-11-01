/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    width:  mainCol.width  + (ScreenTools.defaultFontPixelWidth  * 2)
    height: mainCol.height + (ScreenTools.defaultFontPixelHeight * 2)

    readonly property real axisMonitorWidth: ScreenTools.defaultFontPixelWidth * 32

    property bool _buttonsOnly: _activeJoystick.axisCount == 0
    property bool _requiresCalibration: !_activeJoystick.calibrated && !_buttonsOnly
    property var _activeVehicle: globals.activeVehicle

    Column {
        id:                 mainCol
        anchors.centerIn:   parent
        spacing:            ScreenTools.defaultFontPixelHeight
        GridLayout {
            columns:            2
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         ScreenTools.defaultFontPixelHeight
            //---------------------------------------------------------------------
            //-- Enable Joystick
            QGCLabel {
                text:               _requiresCalibration ? qsTr("Enable not allowed (Calibrate First)") : qsTr("Enable joystick input")
                Layout.alignment:   Qt.AlignVCenter
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 36
            }
            QGCCheckBox {
                id:             enabledSwitch
                enabled:        !_requiresCalibration
                onClicked:      {
                    _activeVehicle.joystickEnabled = checked
                    _activeVehicle.saveJoystickSettings()
                }
                Component.onCompleted: {
                    checked = _activeVehicle.joystickEnabled
                }
                Connections {
                    target: _activeVehicle
                    onJoystickEnabledChanged: {
                        enabledSwitch.checked = _activeVehicle.joystickEnabled
                    }
                }
                Connections {
                    target: joystickManager
                    onActiveJoystickChanged: {
                        if(_activeJoystick) {
                            enabledSwitch.checked = Qt.binding(function() { return _activeJoystick.calibrated && _activeVehicle.joystickEnabled })
                        }
                    }
                }
            }
            //---------------------------------------------------------------------
            //-- Joystick Selector
            QGCLabel {
                text:               qsTr("Active joystick:")
                Layout.alignment:   Qt.AlignVCenter
            }
            QGCComboBox {
                id:                 joystickCombo
                width:              ScreenTools.defaultFontPixelWidth * 40
                Layout.alignment:   Qt.AlignVCenter
                model:              joystickManager.joystickNames
                onActivated: (index) => { joystickManager.activeJoystickName = textAt(index) }
                Component.onCompleted: {
                    var index = joystickCombo.find(joystickManager.activeJoystickName)
                    if (index === -1) {
                        console.warn(qsTr("Active joystick name not in combo"), joystickManager.activeJoystickName)
                    } else {
                        joystickCombo.currentIndex = index
                    }
                }
                Connections {
                    target: joystickManager
                    onAvailableJoysticksChanged: {
                        var index = joystickCombo.find(joystickManager.activeJoystickName)
                        if (index >= 0) {
                            joystickCombo.currentIndex = index
                        }
                    }
                }
            }
            //---------------------------------------------------------------------
            //-- RC Mode
            QGCLabel {
                text:               qsTr("RC Mode:")
                Layout.alignment:   Qt.AlignVCenter
                visible:            !_buttonsOnly
            }
            Row {
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            !_buttonsOnly
                QGCRadioButton {
                    text:       "1"
                    checked:    controller.transmitterMode === 1
                    enabled:    !controller.calibrating
                    onClicked:  controller.transmitterMode = 1
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCRadioButton {
                    text:       "2"
                    checked:    controller.transmitterMode === 2
                    enabled:    !controller.calibrating
                    onClicked:  controller.transmitterMode = 2
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCRadioButton {
                    text:       "3"
                    checked:    controller.transmitterMode === 3
                    enabled:    !controller.calibrating
                    onClicked:  controller.transmitterMode = 3
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCRadioButton {
                    text:       "4"
                    checked:    controller.transmitterMode === 4
                    enabled:    !controller.calibrating
                    onClicked:  controller.transmitterMode = 4
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        Row {
            spacing:                ScreenTools.defaultFontPixelWidth
            //---------------------------------------------------------------------
            //-- Axis Monitors
            Rectangle {
                id:                 axisRect
                color:              Qt.rgba(0,0,0,0)
                border.color:       qgcPal.text
                border.width:       1
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                width:              axisGrid.width  + (ScreenTools.defaultFontPixelWidth  * 2)
                height:             axisGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                visible:            !_buttonsOnly
                GridLayout {
                    id:                 axisGrid
                    columns:            2
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    rowSpacing:         ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent

                    QGCLabel {
                        text:               _activeVehicle.sub ? qsTr("Lateral") : qsTr("Roll")
                        Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 12
                    }
                    AxisMonitor {
                        id:                 rollAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.rollAxisMapped
                        reversed:           controller.rollAxisReversed
                    }

                    QGCLabel {
                        id:                 pitchLabel
                        width:              _attitudeLabelWidth
                        text:               _activeVehicle.sub ? qsTr("Forward") : qsTr("Pitch")
                    }
                    AxisMonitor {
                        id:                 pitchAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.pitchAxisMapped
                        reversed:           controller.pitchAxisReversed
                    }

                    QGCLabel {
                        id:                 yawLabel
                        width:              _attitudeLabelWidth
                        text:               qsTr("Yaw")
                    }
                    AxisMonitor {
                        id:                 yawAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.yawAxisMapped
                        reversed:           controller.yawAxisReversed
                    }

                    QGCLabel {
                        id:                 throttleLabel
                        width:              _attitudeLabelWidth
                        text:               qsTr("Throttle")
                    }
                    AxisMonitor {
                        id:                 throttleAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.throttleAxisMapped
                        reversed:           controller.throttleAxisReversed
                    }

                    QGCLabel {
                        id:                 gimbalPitchLabel
                        enabled:            gimbalPitchAxis.enabled
                        width:              _attitudeLabelWidth
                        text:               qsTr("Gimbal Pitch")
                    }
                    AxisMonitor {
                        id:                 gimbalPitchAxis
                        enabled:            _activeJoystick.axisCount >= 5
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.gimbalPitchAxisMapped
                        reversed:           controller.gimbalPitchAxisReversed
                    }

                    QGCLabel {
                        id:                 gimbalYawLabel
                        enabled:            gimbalYawAxis.enabled
                        width:              _attitudeLabelWidth
                        text:               qsTr("Gimbal Yaw")
                    }
                    AxisMonitor {
                        id:                 gimbalYawAxis
                        enabled:            _activeJoystick.axisCount >= 6
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.gimbalYawAxisMapped
                        reversed:           controller.gimbalYawAxisReversed
                    }

                    Connections {
                        target: _activeJoystick
                        onAxisValuesUpdated: (roll, pitch, yaw, throttle, gimbalPitch, gimbalYaw) => {
                            rollAxis.axisValue = roll * 32768.0
                            pitchAxis.axisValue = pitch * 32768.0
                            yawAxis.axisValue = yaw * 32768.0
                            throttleAxis.axisValue  = _activeJoystick.negativeThrust ? (throttle * -32768.0) : ((-2 * throttle + 1) * 32768.0)
                            gimbalPitchAxis.axisValue = gimbalPitch * 32768.0
                            gimbalYawAxis.axisValue = gimbalYaw * 32768.0
                        }
                    }
                }
            }

            Rectangle {
                color:              Qt.rgba(0,0,0,0)
                border.color:       qgcPal.text
                border.width:       1
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                width:              axisRect.width
                height:             axisRect.height
                Flow {
                    width:              ScreenTools.defaultFontPixelWidth * 30
                    spacing:            -1
                    anchors.centerIn:   parent
                    Connections {
                        target:     _activeJoystick
                        onRawButtonPressedChanged: (index, pressed) => {
                            if (buttonMonitorRepeater.itemAt(index)) {
                                buttonMonitorRepeater.itemAt(index).pressed = pressed
                            }
                        }
                    }
                    Repeater {
                        id:         buttonMonitorRepeater
                        model:      _activeJoystick ? _activeJoystick.totalButtonCount : []
                        Rectangle {
                            width:          ScreenTools.defaultFontPixelHeight * 1.5
                            height:         width
                            border.width:   1
                            border.color:   qgcPal.text
                            color:          pressed ? qgcPal.buttonHighlight : qgcPal.windowShade
                            property bool pressed
                            QGCLabel {
                                anchors.fill:           parent
                                color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                text:                   modelData
                            }
                        }
                    }
                }
            }
        }
    }
}


