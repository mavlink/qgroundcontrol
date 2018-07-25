import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
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
    showHelp:   true

    signal updated()

    property Fact   fact: null

    property string _validateString

    inputMethodHints: ((fact && fact.typeIsString) || ScreenTools.isiOS) ?
                          Qt.ImhNone :                // iOS numeric keyboard has no done button, we can't use it
                          Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard

    onEditingFinished: {
        if (typeof qgcView !== 'undefined' && qgcView) {
            var errorString = fact.validate(text, false /* convertOnly */)
            if (errorString === "") {
                fact.value = text
                _textField.updated()
            } else {
                _validateString = text
                qgcView.showDialog(validationErrorDialogComponent, qsTr("Invalid Value"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            }
        } else {
            fact.value = text
            fact.valueChanged(fact.value)
            _textField.updated()
        }
    }

    onHelpClicked: qgcView.showDialog(helpDialogComponent, qsTr("Value Details"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)


    Component {
        id: validationErrorDialogComponent

        ParameterEditorDialog {
            validate:       true
            validateValue:  _validateString
            fact:           _textField.fact
        }
    }

    Component {
        id: helpDialogComponent

        ParameterEditorDialog {
            fact: _textField.fact
        }
    }
}
