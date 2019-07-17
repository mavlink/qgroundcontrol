/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    width:                  availableWidth
    height:                 (activeVehicle.supportsJSButton ? buttonCol.height : buttonFlow.height) + (ScreenTools.defaultFontPixelHeight * 2)
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
    Flow {
        id:                 buttonFlow
        width:              parent.width
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            !activeVehicle.supportsJSButton
        anchors.centerIn:   parent
        Repeater {
            id:             buttonActionRepeater
            model:          _activeJoystick ? Math.min(_activeJoystick.totalButtonCount, _maxButtons) : []
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                property bool pressed
                QGCCheckBox {
                    anchors.verticalCenter:     parent.verticalCenter
                    checked:                    _activeJoystick ? _activeJoystick.buttonActions[modelData] !== "" : false
                    onClicked:                  _activeJoystick.setButtonAction(modelData, checked ? buttonActionCombo.textAt(buttonActionCombo.currentIndex) : "")
                }
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
                    width:                      ScreenTools.defaultFontPixelWidth * 20
                    model:                      _activeJoystick ? _activeJoystick.actions : 0
                    onActivated:                _activeJoystick.setButtonAction(modelData, textAt(index))
                    Component.onCompleted:      currentIndex = find(_activeJoystick.buttonActions[modelData])
                }
            }
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


