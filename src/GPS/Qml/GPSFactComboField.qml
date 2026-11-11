pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl.FactControls

/// A Fact chosen from its values, under its label.
GPSFactField {
    id: root

    property alias comboBox: combo

    FactComboBox {
        id: combo
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        fact: root.fact
    }
}
