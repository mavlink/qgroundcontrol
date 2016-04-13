/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick          2.5
import QtQuick.Controls 1.3

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

QGCViewDialog {
    id: root

    property Fact   fact
    property bool   showRCToParam:  false
    property bool   validate:       false
    property string validateValue

    ParameterEditorController { id: controller; factPanel: parent }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function accept() {
        if (factCombo.visible) {
            fact.enumIndex = factCombo.currentIndex
            hideDialog()
        } else {
            var errorString = fact.validate(valueField.text, forceSave.checked)
            if (errorString == "") {
                fact.value = valueField.text
                fact.valueChanged(fact.value)
                hideDialog()
            } else {
                validationError.text = errorString
                forceSave.visible = true
            }
        }
    }

    Component.onCompleted: {
        if (validate) {
            validationError.text = fact.validate(validateValue, false /* convertOnly */)
            forceSave.visible = true
        }
        //valueField.forceActiveFocus()
    }

    QGCFlickable {
        anchors.fill:       parent
        contentHeight:      _column.y + _column.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:             _column
            spacing:        defaultTextHeight
            anchors.left:   parent.left
            anchors.right:  parent.right

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       fact.shortDescription ? fact.shortDescription : qsTr("Description missing")
            }

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                visible:    fact.longDescription
                text:       fact.longDescription
            }

            Row {
                spacing: defaultTextWidth

                QGCTextField {
                    id:         valueField
                    text:       validate ? validateValue : fact.valueString
                    visible:    fact.enumStrings.length == 0 || validate
                    //focus:  true

                    // At this point all Facts are numeric
                    inputMethodHints:   Qt.ImhFormattedNumbersOnly
                }

                QGCButton {
                    anchors.baseline:   valueField.baseline
                    visible:            fact.defaultValueAvailable
                    text:               qsTr("Reset to default")

                    onClicked: {
                        fact.value = fact.defaultValue
                        fact.valueChanged(fact.value)
                        hideDialog()
                    }
                }
            }

            QGCComboBox {
                id:             factCombo
                width:          valueField.width
                visible:        _showCombo
                model:          fact.enumStrings

                property bool _showCombo: fact.enumStrings.length != 0 && !validate

                Component.onCompleted: {
                    // We can't bind directly to fact.enumIndex since that would add an unknown value
                    // if there are no enum strings.
                    if (_showCombo) {
                        currentIndex = fact.enumIndex
                    }
                }
            }

            QGCLabel { text: fact.name }

            Row {
                spacing: defaultTextWidth

                QGCLabel { text: qsTr("Units:") }
                QGCLabel { text: fact.units ? fact.units : qsTr("none") }
            }

            Row {
                spacing: defaultTextWidth
                visible: !fact.minIsDefaultForType

                QGCLabel { text: qsTr("Minimum value:") }
                QGCLabel { text: fact.minString }
            }

            Row {
                spacing: defaultTextWidth
                visible: !fact.maxIsDefaultForType

                QGCLabel { text: qsTr("Maximum value:") }
                QGCLabel { text: fact.maxString }
            }

            Row {
                spacing: defaultTextWidth

                QGCLabel { text: qsTr("Default value:") }
                QGCLabel { text: fact.defaultValueAvailable ? fact.defaultValueString : qsTr("none") }
            }

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       qsTr("Warning: Modifying parameters while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
                            qsTr("Make sure you know what you are doing and double-check your values before Save!")
            }

            QGCLabel {
                id:         validationError
                width:      parent.width
                wrapMode:   Text.WordWrap
                color:      qsTr("yellow")
            }

            QGCCheckBox {
                id:         forceSave
                visible:    false
                text:       qsTr("Force save (dangerous!)")
            }

            Row {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth / 2
                visible:    showRCToParam

                Rectangle {
                    height: 1
                    width:  ScreenTools.defaultFontPixelWidth * 5
                    color:  qgcPal.text
                    anchors.verticalCenter: _advanced.verticalCenter
                }

                QGCCheckBox {
                    id:     _advanced
                    text:   qsTr("Advanced settings")
                }

                Rectangle {
                    height: 1
                    width:  ScreenTools.defaultFontPixelWidth * 5
                    color:  qgcPal.text
                    anchors.verticalCenter: _advanced.verticalCenter
                }
            }

            QGCButton {
                text:           qsTr("Set RC to Param...")
                visible:        _advanced.checked && !validate && showRCToParam
                onClicked:      controller.setRCToParam(fact.name)
            }
        } // Column
    }
} // QGCViewDialog
