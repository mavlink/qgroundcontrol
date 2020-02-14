/****************************************************************************
 *
 *   (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11
import QtQuick.Extras               1.4

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    id: joystickConfigButtonsItem
    width:                  availableWidth
    height:                 (activeVehicle.supportsJSButton ? buttonCol.height : buttonFlow.height) + (ScreenTools.defaultFontPixelHeight * 2) + addComboLine.height
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

    Connections {
        target: _activeJoystick
        onButtonRepeatChanged: {
            buttonActionRepeater.itemAt(buttonIndex).repeatCheckAlias = set;
        }
        onButtonCustomActionChanged: {
            buttonComboActionRepeater.itemAt(customActionIndex).comboEnabledAlias = set;
        }
    }

    Flow {
        id:                 buttonFlow
        width:              parent.width
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            !activeVehicle.supportsJSButton
        anchors.top: joystickConfigButtonsItem.top
        anchors.left: joystickConfigButtonsItem.left
        anchors.topMargin: 20
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight

        Repeater {
            id:             buttonActionRepeater
            model:          _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : []

            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                property bool pressed
                property alias repeatCheckAlias: repeatCheck.checked
                property var  currentAssignableAction: _activeJoystick ? _activeJoystick.getAssignableAction(buttonActionCombo.currentText) : null
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
                    onActivated: {
                        _activeJoystick.setButtonAction(modelData, textAt(index), true)
                    }
                    Component.onCompleted: {
                        if(_activeJoystick) {
                            var i = find(_activeJoystick.buttonActions[modelData])
                            if(i < 0) i = 0
                            currentIndex = i
                        }
                    }
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
        Repeater {
            id:             buttonComboActionRepeater
            // The model is a list of JoystickComboAction
            model:          _activeJoystick ? _activeJoystick.customActions : null
            visible:        model.length > 0
            property variant secondaryActionTypes : [ "NONE", "DOUBLE", "LONG", "COMBO" ]
            Component.onCompleted: {
                // We create a list of combos during initialization to make it easier to display in the UI
                _activeJoystick.createAvailableButtonsList()
                _activeJoystick.createCustomActionsTable()
            }
            Row {
                property int _repeaterIndex: index
                spacing:    ScreenTools.defaultFontPixelWidth
                property alias comboEnabledAlias: joystickCustomActionIndicator.comboEnabled
                property bool pressed
                property int _undefinedButton : 0
                Rectangle {
                    id: joystickCustomActionIndicator
                    anchors.verticalCenter:     parent.verticalCenter
                    width:                      ScreenTools.defaultFontPixelHeight * 1.5
                    height:                     width
                    border.width:               1
                    border.color:               qgcPal.text
                    property bool comboEnabled
                    color: comboEnabled ? "green" : "red"
                    Component.onCompleted: {
                        if(_activeJoystick) {
                            comboEnabled = modelData.secondaryActionType !== 0
                        }
                    }
                    QGCLabel {
                        anchors.fill:           parent
                        color:                  pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        text:                   "C" + _repeaterIndex
                    }
                }
                QGCComboBox {
                    id:                         buttonSecondaryActionCombo
                    width:                      ScreenTools.defaultFontPixelWidth * 26
                    model:                      _activeJoystick ? _activeJoystick.assignableActionTitles : []
                    onActivated: {
                        _activeJoystick.updateCustomActions(_repeaterIndex, joystickComboItem1.textAt(joystickComboItem1.currentIndex), joystickComboItem2.textAt(joystickComboItem2.currentIndex), textAt(index), joystickSecondaryActionType.currentIndex)
                    }
                    Component.onCompleted: {
                        if(_activeJoystick) {
                            var i = find(modelData.action)
                            if(i < 0) i = 0
                            currentIndex = i
                        }
                    }
                }
                QGCComboBox {
                    id:                         joystickSecondaryActionType
                    width:                      ScreenTools.defaultFontPixelWidth * 26
                    model:                      _activeJoystick ? buttonComboActionRepeater.secondaryActionTypes : []
                    onActivated: {
                        _activeJoystick.updateCustomActions(_repeaterIndex, joystickComboItem1.textAt(joystickComboItem1.currentIndex), joystickComboItem2.textAt(joystickComboItem2.currentIndex), buttonSecondaryActionCombo.textAt(buttonSecondaryActionCombo.currentIndex), index)
                    }
                    Component.onCompleted: {
                        if(_activeJoystick) {
                            var i = find(_activeJoystick.buttonSecondaryActionTypeList[modelData.secondaryActionType])
                            if(i < 0) i = 0
                            currentIndex = i
                        }
                    }
                }
                QGCComboBox {
                    id:                         joystickComboItem1
                    width:                      ScreenTools.defaultFontPixelWidth * 26
                    model:                      _activeJoystick ? modelData.buttonOptions : []
                    onActivated: {
                        _activeJoystick.updateCustomActions(_repeaterIndex, textAt(index), joystickComboItem2.textAt(joystickComboItem2.currentIndex), buttonSecondaryActionCombo.textAt(buttonSecondaryActionCombo.currentIndex), joystickSecondaryActionType.currentIndex)
                    }
                    Component.onCompleted: {
                        if(_activeJoystick) {
                            var i = find(modelData.buttonOptions[modelData.selectedButtonIndex1])
                            if(i < 0)
                                i = 0
                            currentIndex = i
                        }
                    }
                    Connections {
                        target: modelData
                        onButtonOptionsChanged: {
                            if(_activeJoystick && modelData.selectedButtonIndex1 != 0) {
                                var i = joystickComboItem1.find(modelData.buttonOptions[modelData.selectedButtonIndex1])
                                if(i < 0)
                                    i = 0
                                joystickComboItem1.currentIndex = i
                            }
                        }
                    }
                }
                Image {
                    id:                 comboButtonPlus
                    source:             "/qmlimages/MapAddMission.svg"
                    visible:            joystickSecondaryActionType.currentIndex === 3
                    width:              ScreenTools.defaultFontPixelHeight * 1.5
                    height:             width
                }
                QGCComboBox {
                    id:                         joystickComboItem2
                    width:                      ScreenTools.defaultFontPixelWidth * 26
                    model:                      _activeJoystick ? modelData.buttonOptions : []
                    visible:                    joystickSecondaryActionType.currentIndex === 3
                    onActivated: {
                        _activeJoystick.updateCustomActions(_repeaterIndex, joystickComboItem1.textAt(joystickComboItem1.currentIndex), textAt(index), buttonSecondaryActionCombo.textAt(buttonSecondaryActionCombo.currentIndex), joystickSecondaryActionType.currentIndex)
                    }
                    Component.onCompleted: {
                        if(_activeJoystick && modelData.selectedButtonIndex2 != 0) {
                            var i = find(modelData.buttonOptions[modelData.selectedButtonIndex2])
                            if(i < 0)
                                i = 0
                            currentIndex = i
                        }
                    }
                    Connections {
                        target: modelData
                        onButtonOptionsChanged: {
                            if(_activeJoystick && modelData.selectedButtonIndex2 != 0) {
                                var i = joystickComboItem2.find(modelData.buttonOptions[modelData.selectedButtonIndex2])
                                if(i < 0)
                                    i = 0
                                joystickComboItem2.currentIndex = i
                            }
                        }
                    }
                }
                Image {
                    id:                 comboButtonX
                    source:             "/custom/img/PairingDelete.svg"
                    width:              ScreenTools.defaultFontPixelHeight * 1.5
                    height:             width
                    MouseArea {
                        anchors.fill:       parent
                        onClicked: {
                            _activeJoystick.removeCustomAction(_repeaterIndex)
                        }
                    }
                }
            }
        }
    }
    //TODO: disable button if the last combo in not completed
    QGCButton {
        id: addComboLine
        anchors.top: buttonFlow.bottom
        anchors.left: buttonFlow.left
        anchors.margins: ScreenTools.defaultFontPixelHeight
        text: "Add Custom Action"
        visible: true
        onClicked: {
            if(_activeJoystick)
                _activeJoystick.newCustomAction();
        }
    }
    
    Column {
        id:         buttonCol
        width:      parent.width
        visible:    activeVehicle.supportsJSButton
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
                visible: activeVehicle.supportsJSButton

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
                    id:         mainJSButtonActionCombo
                    width:      ScreenTools.defaultFontPixelWidth * 15
                    fact:       controller.parameterExists(-1, "BTN"+index+"_FUNCTION") ? controller.getParameterFact(-1, "BTN" + index + "_FUNCTION") : null;
                    indexModel: false
                }

                FactComboBox {
                    id:         shiftJSButtonActionCombo
                    width:      ScreenTools.defaultFontPixelWidth * 15
                    fact:       controller.parameterExists(-1, "BTN"+index+"_SFUNCTION") ? controller.getParameterFact(-1, "BTN" + index + "_SFUNCTION") : null;
                    indexModel: false
                }
            }
        }
    }
}
