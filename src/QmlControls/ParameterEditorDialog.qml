/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

    property real   _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 20

    ParameterEditorController { id: controller; factPanel: parent }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function accept() {
        if (bitmaskColumn.visible) {
            var value = 0;
            for (var i = 0; i < fact.bitmaskValues.length; ++i) {
                var checkbox = bitmaskRepeater.itemAt(i)
                if (checkbox.checked) {
                    value |= fact.bitmaskValues[i];
                }
            }
            fact.value = value;
            fact.valueChanged(fact.value)
            hideDialog();
        } else if (factCombo.visible) {
            fact.enumIndex = factCombo.currentIndex
            hideDialog()
        } else {
            var errorString = fact.validate(valueField.text, forceSave.checked)
            if (errorString === "") {
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
                visible:    fact.shortDescription
                text:       fact.shortDescription
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

                    inputMethodHints:   ScreenTools.isiOS ?
                                            Qt.ImhNone :                // iOS numeric keyboard has not done button, we can't use it
                                            Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard
                }

                QGCButton {
                    anchors.baseline:   valueField.baseline
                    visible:            fact.defaultValueAvailable
                    width:              _editFieldWidth
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
                anchors.left:   parent.left
                anchors.right:  parent.right
                visible:        _showCombo
                model:          fact.enumStrings

                property bool _showCombo: fact.enumStrings.length != 0 && fact.bitmaskStrings.length == 0 && !validate

                Component.onCompleted: {
                    // We can't bind directly to fact.enumIndex since that would add an unknown value
                    // if there are no enum strings.
                    if (_showCombo) {
                        currentIndex = fact.enumIndex
                    }
                }
            }

            Column {
                id:         bitmaskColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2
                visible:    fact.bitmaskStrings.length > 0 ? true : false;

                Repeater {
                    id:     bitmaskRepeater
                    model:  fact.bitmaskStrings

                    delegate : QGCCheckBox {
                        text : modelData
                        checked : fact.value & fact.bitmaskValues[index]
                    }
                }
            }

            QGCLabel {
                text:       fact.name
                visible:    fact.componentId > 0 // > 0 means it's a parameter fact
            }

            Column {
                spacing:        defaultTextHeight / 2
                anchors.left:   parent.left
                anchors.right:  parent.right

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
                    visible:    fact.rebootRequired
                    text:       "Reboot required after change"
                }
            } // Column

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       qsTr("Warning: Modifying values while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
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
                width:          _editFieldWidth
                visible:        _advanced.checked && !validate && showRCToParam
                onClicked:      controller.setRCToParam(fact.name)
            }
        } // Column
    }
} // QGCViewDialog
