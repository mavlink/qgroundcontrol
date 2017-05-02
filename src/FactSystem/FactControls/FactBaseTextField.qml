import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCTextField {
    id: _textField

    text:       fact ? fact.valueString : ""
    unitsLabel: fact ? fact.units : ""
    showUnits:  true
    showHelp:   true

    property Fact   fact:           null

    // At this point all Facts are numeric
    validator:          DoubleValidator {}
    inputMethodHints:   ScreenTools.isiOS ?
                            Qt.ImhNone :                // iOS numeric keyboard has not done button, we can't use it
                            Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard

    onEditingFinished: {
        if (typeof qgcView !== 'undefined' && qgcView) {
            var errorText = fact.validate(text, false /* convertOnly */)
            if (errorText === "") {
                setFactValue(text)
            } else {
                validationError(text, errorText)
            }
        } else {
            setFactValue(text)
        }
    }

    function setFactValue(newValue) {
        setFactValueImpl(newValue)
    }

    // Extra layer of indirections to allow 'super' call if setFactValue is overriden
    function setFactValueImpl(newValue) {
        fact.value = newValue
        fact.valueChanged(fact.value) //? Is this really needed? Fact.cc emits value change signal already
        valueChanged(newValue)
    }

    signal valueChanged(string text)
    signal validationError(string text, string errorText)
}
