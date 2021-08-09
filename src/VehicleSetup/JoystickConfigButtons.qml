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

ColumnLayout {
    width:                  availableWidth
    height:                 (globals.activeVehicle.supportsJSButton ? buttonCol.height : flowColumn.height) + (ScreenTools.defaultFontPixelHeight * 2)
    Connections {
        target: _activeJoystick
        onRawButtonPressedChanged: {
            if (buttonActionRepeater.itemAt(index)) {
                buttonActionRepeater.itemAt(index).pressed = pressed
            }
            if (jsButtonActionRepeater.itemAt(index)) {
                jsButtonActionRepeater.itemAt(index).pressed = pressed
            }
        }
    }
    ColumnLayout {
        id:         flowColumn
        y:          ScreenTools.defaultFontPixelHeight / 2
        width:      parent.width
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        QGCLabel {
            Layout.preferredWidth:  parent.width
            wrapMode:               Text.WordWrap
            text:                   qsTr("Assigning the same action to multiple buttons requires the press of all those buttons for the action to be taken. This is useful to prevent accidental button presses for critical actions like Arm or Emergency Stop.")
        }
        Flow {
            id:                     buttonFlow
            Layout.preferredWidth:  parent.width
            spacing:                ScreenTools.defaultFontPixelWidth
            visible:                !globals.activeVehicle.supportsJSButton
            Repeater {
                id:             buttonActionRepeater
                model:          _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : []
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    property bool pressed
                    property var  currentAssignableAction: _activeJoystick ? _activeJoystick.assignableActions.get(buttonActionCombo.currentIndex) : null
                    Rectangle {
                        anchors.verticalCenter:     parent.verticalCenter
                        width:                      ScreenTools.defaultFontPixelHeight * 1.5
                        height:                     width
                        border.width:               1
                        border.color:               qgcPal.text
                        color:                      pressed ? qgcPal.buttonHighlight : qgcPal.button
                        QGCLabel {
                            anchors.fill:           parent
                            color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            text:                   modelData
                        }
                    }
                    QGCComboBox {
                        id:                         buttonActionCombo
                        width:                      ScreenTools.defaultFontPixelWidth * 26
                        model:                      _activeJoystick ? _activeJoystick.assignableActionTitles : []
                        sizeToContents:             true

                        function _findCurrentButtonAction() {
                            if(_activeJoystick) {
                                var i = find(_activeJoystick.buttonActions[modelData])
                                if(i < 0) i = 0
                                currentIndex = i
                            }
                        }

                        Component.onCompleted:  _findCurrentButtonAction()
                        onModelChanged:         _findCurrentButtonAction()
                        onActivated:            _activeJoystick.setButtonAction(modelData, textAt(index))
                    }
                    QGCCheckBox {
                        id:                         repeatCheck
                        text:                       qsTr("Repeat")
                        enabled:                    currentAssignableAction && _activeJoystick.calibrated && currentAssignableAction.canRepeat
                        onClicked: {
                            _activeJoystick.setButtonRepeat(modelData, checked)
                        }
                        Component.onCompleted: {
                            if(_activeJoystick) {
                                checked = _activeJoystick.getButtonRepeat(modelData)
                            }
                        }
                        anchors.verticalCenter:     parent.verticalCenter
                    }
                    Item {
                        width:                      ScreenTools.defaultFontPixelWidth * 2
                        height:                     1
                    }
                }
            }
        }
    }
    Column {
        id:         buttonCol
        width:      parent.width
        visible:    globals.activeVehicle.supportsJSButton
        spacing:    ScreenTools.defaultFontPixelHeight / 3
        Row {
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel {
                horizontalAlignment:    Text.AlignHCenter
                width:                  ScreenTools.defaultFontPixelHeight * 1.5
                text:                   qsTr("#")
            }
            QGCLabel {
                width:                  ScreenTools.defaultFontPixelWidth * 15
                text:                   qsTr("Function: ")
            }
            QGCLabel {
                width:                  ScreenTools.defaultFontPixelWidth * 15
                text:                   qsTr("Shift Function: ")
            }
        }
        Repeater {
            id:     jsButtonActionRepeater
            model:  _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : 0

            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                visible: globals.activeVehicle.supportsJSButton
                property var parameterName: `BTN${index}_FUNCTION`
                property var parameterShiftName: `BTN${index}_SFUNCTION`
                property bool hasFirmwareSupport: controller.parameterExists(-1, parameterName)

                property bool pressed

                Rectangle {
                    anchors.verticalCenter:     parent.verticalCenter
                    width:                      ScreenTools.defaultFontPixelHeight * 1.5
                    height:                     width
                    border.width:               1
                    border.color:               qgcPal.text
                    color:                      pressed ? qgcPal.buttonHighlight : qgcPal.button


                    QGCLabel {
                        anchors.fill:           parent
                        color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        text:                   modelData
                    }
                }

                FactComboBox {
                    id:             mainJSButtonActionCombo
                    width:          ScreenTools.defaultFontPixelWidth * 15
                    fact:           hasFirmwareSupport ? controller.getParameterFact(-1, parameterName) : null;
                    visible:        hasFirmwareSupport
                    indexModel:     false
                    sizeToContents: true
                }

                FactComboBox {
                    id:             shiftJSButtonActionCombo
                    width:          ScreenTools.defaultFontPixelWidth * 15
                    fact:           hasFirmwareSupport ? controller.getParameterFact(-1, parameterShiftName) : null;
                    visible:        hasFirmwareSupport
                    indexModel:     false
                    sizeToContents: true
                }

                QGCLabel {
                    text:                   qsTr("No firmware support")
                    width:                  ScreenTools.defaultFontPixelWidth * 15
                    visible:                !hasFirmwareSupport
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}


