import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

QGCCheckBoxSlider {
    property Fact fact: Fact { }

    property var checkedValue:   _typeIsBool ? true : 1
    property var uncheckedValue: _typeIsBool ? false : 0

    property var _typeIsBool: fact ? fact.typeIsBool : false

    Binding on checked {
        value: fact ?
                (fact.value === uncheckedValue ? Qt.Unchecked : Qt.Checked) :
                Qt.Unchecked
    }

    onClicked: fact.value = (checked ? checkedValue : uncheckedValue)
}
