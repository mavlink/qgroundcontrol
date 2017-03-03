import QtQuick                  2.7
import QtQuick.Controls         2.1
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
    showHelp:   true

    property Fact   fact: null

    property string _validateString

    // At this point all Facts are numeric
    inputMethodHints: (fact.typeIsString || ScreenTools.isiOS) ?
                          Qt.ImhNone :                // iOS numeric keyboard has no done button, we can't use it
                          Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard

    onEditingFinished: {
        if (ScreenTools.isMobile) {
            // Toss focus on mobile after Done on virtual keyboard. Prevent strange interactions.
            focus = false
        }
        if (typeof qgcView !== 'undefined' && qgcView) {
            var errorString = fact.validate(text, false /* convertOnly */)
            if (errorString == "") {
                fact.value = text
            } else {
                _validateString = text
                qgcView.showDialog(validationErrorDialogComponent, qsTr("Invalid Value"), qgcView.showDialogDefaultWidth, StandardButton.Save)
            }
        } else {
            fact.value = text
            fact.valueChanged(fact.value)
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
