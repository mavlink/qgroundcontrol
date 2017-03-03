import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCCheckBox {
    property Fact fact: Fact { }
    property variant checkedValue: 1
    property variant uncheckedValue: 0

    checkedState: fact ?
                      (fact.typeIsBool ?
                           (fact.value === true ? Qt.Checked : Qt.Unchecked) :
                           (fact.value === checkedValue ? Qt.Checked : Qt.Unchecked)) :
                      Qt.Unchecked

    text: qsTr("Label")

    onClicked: fact.value = checked ? checkedValue : uncheckedValue
}
