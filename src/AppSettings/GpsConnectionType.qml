import QtQuick

import QGroundControl.Controls

LabelledComboBox {
    id: root
    label: qsTr("Connection")
    required property var fact
    property var excludedValues: []
    readonly property var _values: root.fact.enumValues.filter(value => root.excludedValues.indexOf(value) === -1 || value === root.fact.rawValue)
    model: root._values.map(value => {
        const name = root.fact.enumStrings[root.fact.enumValues.indexOf(value)]
        return root.excludedValues.indexOf(value) === -1 ? name : qsTr("%1 (unavailable)").arg(name)
    })
    currentIndex: root._values.indexOf(root.fact.rawValue)
    onActivated: index => {
        if (index >= 0 && index < root._values.length && root.excludedValues.indexOf(root._values[index]) === -1) {
            root.fact.rawValue = root._values[index]
        }
    }
}
