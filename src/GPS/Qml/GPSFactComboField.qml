pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl.FactControls

/// A Fact chosen from its values, under its label.
GPSFactField {
    id: root

    property alias comboBox: combo

    // Assistive technology names a combo box by its selected value unless it has a name, so the value becomes the
    // description.
    FactComboBox {
        id: combo
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Accessible.name: root.label
        Accessible.description: combo.displayText
        fact: root.fact
    }
}
