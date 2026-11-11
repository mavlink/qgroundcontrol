pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl.FactControls

/// A Fact edited as text, under its label.
GPSFactField {
    id: root

    FactTextField {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Accessible.name: root.label
        fact: root.fact
    }
}
