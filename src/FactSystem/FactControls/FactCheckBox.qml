import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCCheckBox {
    checkedState: isFactChecked()

    property Fact fact: Fact { }

    property variant checkedValue:   1
    property variant uncheckedValue: 0

    Binding on checkedState {
        value: fact ?
                   (fact.typeIsBool ?
                        (fact.value === false ? Qt.Unchecked : Qt.Checked) :
                        (fact.value === 0 ? Qt.Unchecked : Qt.Checked)) :
                   Qt.Unchecked
    }

    onClicked: fact.value = (checked ? checkedValue : uncheckedValue)
}
