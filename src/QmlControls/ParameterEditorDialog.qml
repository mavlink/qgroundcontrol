import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

QGCPopupDialog {
    id:         root
    title:      fact.componentId > 0 ? fact.name : qsTr("Value Editor")

    buttons:    _readOnlyDisplay ? Dialog.Close : (Dialog.Save | Dialog.Cancel)

    property Fact   fact
    property bool   showRCToParam:  false
    property bool   setFocus:       true    ///< true: focus is set to text field on display, false: focus not set (works around strange virtual keyboard bug with FactValueSlider

    property real   _editFieldWidth:            ScreenTools.defaultFontPixelWidth * 20
    property bool   _longDescriptionAvailable:  fact.longDescription != ""
    property bool   _editingParameter:          fact.componentId != 0
    property bool   _allowForceSave:            QGroundControl.corePlugin.showAdvancedUI && _editingParameter
    property bool   _allowDefaultReset:         fact.defaultValueAvailable
    property bool   _showCombo:                 fact.enumStrings.length !== 0 && fact.bitmaskStrings.length === 0
    property bool   _readOnlyDisplay:           fact.readOnly && !forceEdit.checked
    property bool   _allowForceEdit:            fact.readOnly && _allowForceSave
    property real   _rowSpacing:                ScreenTools.defaultFontPixelHeight / 2
    property real   _columnSpacing:             ScreenTools.defaultFontPixelWidth

    ParameterEditorController { id: controller; }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    onAccepted: {
        if (_readOnlyDisplay) return
        if (bitmaskColumn.visible && !manualEntry.checked) {
            fact.value = bitmaskValue();
            fact.valueChanged(fact.value)
        } else if (factCombo.visible && !manualEntry.checked) {
            fact.enumIndex = factCombo.currentIndex
        } else {
            var errorString = fact.validate(valueField.text, forceSave.checked)
            if (errorString === "") {
                fact.value = valueField.text
                fact.valueChanged(fact.value)
            } else {
                validationError.text = errorString
                if (_allowForceSave) {
                    forceSave.visible = true
                }
                preventClose = true
            }
        }
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
        valueField.text = fact.valueString
    }

    ColumnLayout {
        width:      Math.min(mainWindow.width * .75, Math.max(ScreenTools.defaultFontPixelWidth * 60, editRow.width))
        spacing:    _rowSpacing

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            text:               forceEdit.checked
                                    ? qsTr("Warning: This parameter is read-only. Force edit is enabled.")
                                    : qsTr("This parameter is read-only and cannot be modified.")
            color:              forceEdit.checked ? qgcPal.warningText : qgcPal.text
            visible:            fact.readOnly
        }

        QGCLabel {
            id:                 validationError
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            color:              qgcPal.warningText
            visible:            text !== ""
        }

        RowLayout {
            id:         editRow
            spacing:    _columnSpacing
            visible:    !_readOnlyDisplay

            QGCTextField {
                id:                 valueField
                width:              _editFieldWidth
                unitsLabel:         fact.units
                showUnits:          fact.units != ""
                focus:              setFocus && visible
                inputMethodHints:   (fact.typeIsString || ScreenTools.isiOS) ? // iOS numeric keyboard has no done button, we can't use it
                                        Qt.ImhNone :
                                        Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard
                visible:            !_showCombo || manualEntry.checked
            }

            QGCComboBox {
                id:             factCombo
                width:          _editFieldWidth
                model:          fact.enumStrings
                sizeToContents: true
                visible:        _showCombo
                focus:          setFocus && visible

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

            QGCButton {
                visible:    _allowDefaultReset
                text:       qsTr("Reset To Default")

                onClicked: {
                    fact.value = fact.defaultValue
                    fact.valueChanged(fact.value)
                    close()
                }
            }
        }

        QGCLabel {
            visible:    _readOnlyDisplay
            text:       qsTr("Value: ") + fact.valueString + (fact.units != "" ? (" " + fact.units) : "")
        }

        Column {
            id:         bitmaskColumn
            spacing:    _rowSpacing
            visible:    fact.bitmaskStrings.length > 0 && !_readOnlyDisplay

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
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            !longDescriptionLabel.visible
            text:               fact.shortDescription
        }

        QGCLabel {
            id:                 longDescriptionLabel
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            fact.longDescription != ""
            text:               fact.longDescription
        }

        RowLayout {
            spacing: _columnSpacing

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
            visible:    fact.vehicleRebootRequired
            text:       qsTr("Vehicle reboot required after change")
        }

        QGCLabel {
            visible:    fact.qgcRebootRequired
            text:       qsTr("Application restart required after change")
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:   Text.WordWrap
            text:       qsTr("Warning: Modifying values while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
                        qsTr("Make sure you know what you are doing and double-check your values before Save!")
            visible:    fact.componentId != -1 && !_readOnlyDisplay
        }

        QGCCheckBox {
            id:         forceSave
            visible:    false
            text:       qsTr("Force save (dangerous!)")
        }

        QGCCheckBox {
            id:         advancedCheckbox
            text:       qsTr("Advanced settings")
            visible:    _allowForceEdit || (!_readOnlyDisplay && (showRCToParam || factCombo.visible || bitmaskColumn.visible))
        }

        // Force-edit escape hatch for read-only params. Nested under the
        // Advanced settings checkbox so it is never shown without opt-in.
        QGCCheckBox {
            id:         forceEdit
            visible:    _allowForceEdit && advancedCheckbox.checked
            text:       qsTr("Force edit read-only param")

            // Re-lock the param if Advanced settings is unchecked, so edit
            // mode can never persist without the Advanced checkbox checked.
            onVisibleChanged: {
                if (!visible) {
                    checked = false
                }
            }

            // Clear any stale validation/force-save state when the param is re-locked.
            onCheckedChanged: {
                if (!checked) {
                    validationError.text = ""
                    forceSave.visible = false
                }
            }
        }

        // Checkbox to allow manual entry of enumerated or bitmask parameters
        QGCCheckBox {
            id:         manualEntry
            visible:    advancedCheckbox.checked && !_readOnlyDisplay && (factCombo.visible || bitmaskColumn.visible)
            text:       qsTr("Manual Entry")

            onClicked: {
                valueField.text = fact.valueString
            }
        }

        QGCButton {
            text:       qsTr("Set RC to Param")
            visible:    advancedCheckbox.checked && !_readOnlyDisplay && showRCToParam
            onClicked:  rcToParamDialogFactory.open()
        }
    } // Column

    QGCPopupDialogFactory {
        id: rcToParamDialogFactory

        dialogComponent: rcToParamDialog
    }

    Component {
        id: rcToParamDialog

        RCToParamDialog {
            tuningFact: fact
        }
    }
}
