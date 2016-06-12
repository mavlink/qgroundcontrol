import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCTextField {
    id: _textField

    text:       fact ? fact.valueString : ""
    unitsLabel: fact ? fact.units : ""
    showUnits:  true

    property Fact   fact:           null
    property string _validateString

    // At this point all Facts are numeric
    validator:          DoubleValidator {}
    inputMethodHints:   ScreenTools.isiOS ?
                            Qt.ImhNone :                // iOS numeric keyboard has not done button, we can't use it
                            Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard

    onEditingFinished: {
        if (typeof qgcView !== 'undefined' && qgcView) {
            var errorString = fact.validate(text, false /* convertOnly */)
            if (errorString == "") {
                fact.value = text
            } else {
                _validateString = text
                qgcView.showDialog(editorDialogComponent, qsTr("Invalid Parameter Value"), qgcView.showDialogDefaultWidth, StandardButton.Save)
            }
        } else {
            fact.value = text
            fact.valueChanged(fact.value)
        }
    }

    Component {
        id: editorDialogComponent

        ParameterEditorDialog {
            validate:       true
            validateValue:  _validateString
            fact:           _textField.fact
        }
    }
}
