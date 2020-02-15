/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

QGCViewDialog {
    id:     root
    focus:  true

    property Fact   fact
    property bool   showRCToParam:  false
    property bool   validate:       false
    property string validateValue
    property bool   setFocus:       true    ///< true: focus is set to text field on display, false: focus not set (works around strange virtual keyboard bug with FactValueSlider

    signal valueChanged

    property real   _editFieldWidth:            ScreenTools.defaultFontPixelWidth * 20
    property bool   _longDescriptionAvailable:  fact.longDescription != ""
    property bool   _editingParameter:          fact.componentId != 0
    property bool   _allowForceSave:            QGroundControl.corePlugin.showAdvancedUI || !_editingParameter
    property bool   _allowDefaultReset:         fact.defaultValueAvailable && (QGroundControl.corePlugin.showAdvancedUI || !_editingParameter)

    ParameterEditorController { id: controller; }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function accept() {
        if (bitmaskColumn.visible && !manualEntry.checked) {
            fact.value = bitmaskValue();
            fact.valueChanged(fact.value)
            valueChanged()
            hideDialog();
        } else if (factCombo.visible && !manualEntry.checked) {
            fact.enumIndex = factCombo.currentIndex
            valueChanged()
            hideDialog()
        } else {
            var errorString = fact.validate(valueField.text, forceSave.checked)
            if (errorString === "") {
                fact.value = valueField.text
                fact.valueChanged(fact.value)
                valueChanged()
                hideDialog()
            } else {
                validationError.text = errorString
                if (_allowForceSave) {
                    forceSave.visible = true
                }
            }
        }
    }

    function reject() {
        fact.valueChanged(fact.value)
        hideDialog();
    }

    function bitmaskValue() {
        var value = 0;
        for (var i = 0; i < fact.bitmaskValues.length; ++i) {
            var checkbox = bitmaskRepeater.itemAt(i)
            if (checkbox.checked) {
                value |= fact.bitmaskValues[i];
            }
        }
        return value
    }

    Component.onCompleted: {
        if (validate) {
            validationError.text = fact.validate(validateValue, false /* convertOnly */)
            if (_allowForceSave) {
                forceSave.visible = true
            }
        }
    }

    QGCFlickable {
        id:                 flickable
        anchors.fill:       parent
        contentHeight:      _column.y + _column.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:             _column
            spacing:        defaultTextHeight
            anchors.left:   parent.left
            anchors.right:  parent.right

            QGCLabel {
                id:         validationError
                width:      parent.width
                wrapMode:   Text.WordWrap
                color:      qgcPal.warningText
            }

            RowLayout {
                spacing:        ScreenTools.defaultFontPixelWidth
                anchors.left:   parent.left
                anchors.right:  parent.right

                QGCTextField {
                    id:                 valueField
                    text:               validate ? validateValue : fact.valueString
                    visible:            fact.enumStrings.length === 0 || validate || manualEntry.checked
                    unitsLabel:         fact.units
                    showUnits:          fact.units != ""
                    Layout.fillWidth:   true
                    focus:              setFocus
                    inputMethodHints:   (fact.typeIsString || ScreenTools.isiOS) ?
                                          Qt.ImhNone :                // iOS numeric keyboard has no done button, we can't use it
                                          Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard
                }

                QGCButton {
                    visible:    _allowDefaultReset
                    text:       qsTr("Reset to default")

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

                property bool _showCombo: fact.enumStrings.length !== 0 && fact.bitmaskStrings.length === 0 && !validate

                Component.onCompleted: {
                    // We can't bind directly to fact.enumIndex since that would add an unknown value
                    // if there are no enum strings.
                    if (_showCombo) {
                        currentIndex = fact.enumIndex
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex >=0 && currentIndex < model.length) {
                        valueField.text = fact.enumValues[currentIndex]
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

                        onClicked: {
                            valueField.text = bitmaskValue()
                        }
                    }
                }
            }

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                visible:    !longDescriptionLabel.visible
                text:       fact.shortDescription
            }

            QGCLabel {
                id:         longDescriptionLabel
                width:      parent.width
                wrapMode:   Text.WordWrap
                visible:    fact.longDescription != ""
                text:       fact.longDescription
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    id:         minValueDisplay
                    text:       qsTr("Min: ") + fact.minString
                    visible:    !fact.minIsDefaultForType
                }

                QGCLabel {
                    text:       qsTr("Max: ") + fact.maxString
                    visible:    !fact.maxIsDefaultForType
                }

                QGCLabel {
                    text:       qsTr("Default: ") + fact.defaultValueString
                    visible:    _allowDefaultReset
                }
            }

            QGCLabel {
                text:       qsTr("Parameter name: ") + fact.name
                visible:    fact.componentId > 0 // > 0 means it's a parameter fact
            }

            QGCLabel {
                visible:    fact.vehicleRebootRequired
                text:       "Vehicle reboot required after change"
            }

            QGCLabel {
                visible:    fact.qgcRebootRequired
                text:       "Application restart required after change"
            }

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       qsTr("Warning: Modifying values while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
                            qsTr("Make sure you know what you are doing and double-check your values before Save!")
                visible:    fact.componentId != -1
            }

            QGCCheckBox {
                id:         forceSave
                visible:    false
                text:       qsTr("Force save (dangerous!)")
            }

            Row {
                width:      parent.width
                spacing:    ScreenTools.defaultFontPixelWidth / 2
                visible:    showRCToParam || factCombo.visible || bitmaskColumn.visible

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

            // Checkbox to allow manual entry of enumerated or bitmask parameters
            QGCCheckBox {
                id:         manualEntry
                visible:    _advanced.checked && (factCombo.visible || bitmaskColumn.visible)
                text:       qsTr("Manual Entry")

                onClicked: {
                    valueField.text = fact.valueString
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
