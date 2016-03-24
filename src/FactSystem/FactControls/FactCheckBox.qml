import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCCheckBox {
    property Fact fact: Fact { }
    property variant checkedValue: 1
    property variant uncheckedValue: 0

    partiallyCheckedEnabled: fact.value != checkedValue && fact.value != uncheckedValue
    checkedState: fact.value == checkedValue ? Qt.Checked : (fact.value == uncheckedValue ? Qt.Unchecked : Qt.PartiallyChecked)

    text: qsTr("Label")

    onClicked: {
        fact.value = checked ? checkedValue : uncheckedValue
    }
}
