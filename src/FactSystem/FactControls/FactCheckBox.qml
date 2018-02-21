import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCCheckBox {
    property Fact fact: Fact { }

    checkedState: fact ?
                      (fact.typeIsBool ?
                           (fact.value === false ? Qt.Unchecked : Qt.Checked) :
                           (fact.value === 0 ? Qt.Unchecked : Qt.Checked)) :
                      Qt.Unchecked

    text: qsTr("Label")

    onClicked: fact.value = checked ? 1 : 0
}
