import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

FactBaseTextField {
    id: _textField

    property string _validateString
    onValidationError: {
        _validateString = text
        qgcView.showDialog(validationErrorDialogComponent, qsTr("Invalid Value"), qgcView.showDialogDefaultWidth, StandardButton.Save)
    }

    Component {
        id: validationErrorDialogComponent

        ParameterEditorDialog {
            validate:       true
            validateValue:  _validateString
            fact:           _textField.fact
        }
    }

    onHelpClicked: qgcView.showDialog(helpDialogComponent, qsTr("Value Details"), qgcView.showDialogDefaultWidth, StandardButton.Save)

    Component {
        id: helpDialogComponent

        ParameterEditorDialog {
            fact: _textField.fact
        }
    }
}
