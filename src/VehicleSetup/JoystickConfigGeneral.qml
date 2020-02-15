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
    width:                  mainCol.width  + (ScreenTools.defaultFontPixelWidth  * 2)
    height:                 mainCol.height + (ScreenTools.defaultFontPixelHeight * 2)
    readonly property real axisMonitorWidth: ScreenTools.defaultFontPixelWidth * 32
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
                text:               _activeJoystick ? _activeJoystick.calibrated ? qsTr("Enable joystick input") : qsTr("Enable not allowed (Calibrate First)") : ""
                Layout.alignment:   Qt.AlignVCenter
                Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 36
            }
            QGCCheckBox {
                id:             enabledSwitch
                enabled:        _activeJoystick ? _activeJoystick.calibrated : false
                onClicked:      activeVehicle.joystickEnabled = checked
                Component.onCompleted: {
                    checked = activeVehicle.joystickEnabled
                }
                Connections {
                    target: activeVehicle
                    onJoystickEnabledChanged: {
                        enabledSwitch.checked = activeVehicle.joystickEnabled
                    }
                }
                Connections {
                    target: joystickManager
                    onActiveJoystickChanged: {
                        if(_activeJoystick) {
                            enabledSwitch.checked = Qt.binding(function() { return _activeJoystick.calibrated && activeVehicle.joystickEnabled })
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
                onActivated:        joystickManager.activeJoystickName = textAt(index)
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
            }
            Row {
                spacing:            ScreenTools.defaultFontPixelWidth
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
                GridLayout {
                    id:                 axisGrid
                    columns:            2
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    rowSpacing:         ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:               activeVehicle.sub ? qsTr("Lateral") : qsTr("Roll")
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
                        text:               activeVehicle.sub ? qsTr("Forward") : qsTr("Pitch")
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

                    Connections {
                        target:             _activeJoystick
                        onManualControl: {
                            rollAxis.axisValue      = roll  * 32768.0
                            pitchAxis.axisValue     = pitch * 32768.0
                            yawAxis.axisValue       = yaw   * 32768.0
                            throttleAxis.axisValue  = _activeJoystick.negativeThrust ? throttle * -32768.0 : (-2 * throttle + 1) * 32768.0
                        }
                    }

                    QGCLabel {
                        id:                 gimbalPitchLabel
                        width:              _attitudeLabelWidth
                        text:               qsTr("Gimbal Pitch")
                        visible:            controller.hasGimbalPitch && _activeJoystick.gimbalEnabled
                    }
                    AxisMonitor {
                        id:                 gimbalPitchAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.gimbalPitchAxisMapped
                        reversed:           controller.gimbalPitchAxisReversed
                        visible:            controller.hasGimbalPitch && _activeJoystick.gimbalEnabled
                    }

                    QGCLabel {
                        id:                 gimbalYawLabel
                        width:              _attitudeLabelWidth
                        text:               qsTr("Gimbal Yaw")
                        visible:            controller.hasGimbalYaw && _activeJoystick.gimbalEnabled
                    }
                    AxisMonitor {
                        id:                 gimbalYawAxis
                        height:             ScreenTools.defaultFontPixelHeight
                        width:              axisMonitorWidth
                        mapped:             controller.gimbalYawAxisMapped
                        reversed:           controller.gimbalYawAxisReversed
                        visible:            controller.hasGimbalYaw && _activeJoystick.gimbalEnabled
                    }

                    Connections {
                        target:             _activeJoystick
                        onManualControlGimbal:  {
                            gimbalPitchAxis.axisValue = gimbalPitch * 32768.0
                            gimbalYawAxis.axisValue   = gimbalYaw   * 32768.0
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
                        onRawButtonPressedChanged: {
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


