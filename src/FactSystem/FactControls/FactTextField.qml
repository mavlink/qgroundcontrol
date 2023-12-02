import QtQuick
import QtQuick.Controls
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

QGCTextField {
    id: _textField

    text:               fact ? fact.valueString : ""
    unitsLabel:         fact ? fact.units : ""
    showUnits:          true
    showHelp:           true
    numericValuesOnly:  fact && !fact.typeIsString

    signal updated()

    property Fact   fact: null

    property string _validateString

    onEditingFinished: {
        var errorString = fact.validate(text, false /* convertOnly */)
        if (errorString === "") {
            fact.value = text
            _textField.updated()
        } else {
            _validateString = text
            validationErrorDialogComponent.createObject(mainWindow).open()
        }
    }

    onHelpClicked: helpDialogComponent.createObject(mainWindow).open()

    Component {
        id: validationErrorDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Invalid Value")
            validate:       true
            validateValue:  _validateString
            fact:           _textField.fact
        }
    }

    Component {
        id: helpDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Value Details")
            fact:           _textField.fact
        }
    }
}
