import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCCheckBoxSlider {
    property Fact fact: Fact { }

    property var checkedValue:   fact.typeIsBool ? true : 1
    property var uncheckedValue: fact.typeIsBool ? false : 0

    Binding on checked {
        value: fact ?
                (fact.value === uncheckedValue ? Qt.Unchecked : Qt.Checked) :
                Qt.Unchecked
    }

    onClicked: fact.value = (checked ? checkedValue : uncheckedValue)
}
