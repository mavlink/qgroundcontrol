import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

QGCTextField {
    id:                 control
    text:               fact ? fact.valueString : ""
    unitsLabel:         fact ? fact.units : ""
    showUnits:          true
    showHelp:           false
    numericValuesOnly:  fact && !fact.typeIsString

    signal updated()

    property Fact fact: null

    onEditingFinished: _onEditingFinished()
    
    function _onEditingFinished() {
        var errorString = fact.validate(text, false /* convertOnly */)
        if (errorString === "") {
            clearValidationError()
            fact.value = text
            control.updated()
        } else {
            showValidationError(errorString, fact.valueString)
        }
    }

    onHelpClicked: helpDialogComponent.createObject(mainWindow).open()

    Component {
        id: helpDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Value Details")
            fact:           control.fact
        }
    }
}
