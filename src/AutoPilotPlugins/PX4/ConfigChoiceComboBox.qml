import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Combo box over a chosen subset of a Fact's values. Each choice is { label, value }; selecting one sets the Fact's
/// raw value. Use FactComboBox when every metadata choice is shown.
QGCComboBox {
    property var fact:      null
    property var choices:   []

    model:          choices.map(choice => choice.label)
    currentIndex:   _indexOf(fact ? fact.rawValue : undefined)
    sizeToContents: true

    function _indexOf(value) {
        for (let i = 0; i < choices.length; i++) {
            if (Number(choices[i].value) === Number(value)) {
                return i
            }
        }
        return -1
    }

    function _restoreBinding() {
        currentIndex = Qt.binding(() => _indexOf(fact ? fact.rawValue : undefined))
    }

    // A model change resets the index, so restore the binding once the model has settled
    onModelChanged: Qt.callLater(_restoreBinding)

    onActivated: (index) => {
        if (fact && index >= 0 && index < choices.length) {
            fact.rawValue = choices[index].value
        }
        _restoreBinding()
    }
}
