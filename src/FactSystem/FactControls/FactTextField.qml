import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2


import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCTextField {
    id: _textField

    property Fact   fact:           null
    property string _validateString

    text:       fact.valueString
    unitsLabel: fact.units

    onEditingFinished: {
        if (qgcView) {
            var errorString = fact.validate(text, false /* convertOnly */)
            if (errorString == "") {
                fact.value = text
            } else {
                _validateString = text
                qgcView.showDialog(editorDialogComponent, "Invalid Parameter Value", 50, StandardButton.Save)
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
