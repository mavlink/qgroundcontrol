import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.Controls

QGCCheckBox {
    property Fact fact: Fact { }

    property variant checkedValue:   1
    property variant uncheckedValue: 0

    Binding on checkState {
        value: fact ?
                   (fact.typeIsBool ?
                        (fact.value === false ? Qt.Unchecked : Qt.Checked) :
                        (fact.value === 0 ? Qt.Unchecked : Qt.Checked)) :
                   Qt.Unchecked
    }

    onClicked: fact.value = (checked ? checkedValue : uncheckedValue)
}
