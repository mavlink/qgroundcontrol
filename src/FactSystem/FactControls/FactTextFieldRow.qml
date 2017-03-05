import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0

RowLayout {
    property var fact: Fact { }

    QGCLabel {
        text: fact.name + ":"
    }

    FactTextField {
        Layout.fillWidth:   true
        showUnits:          true
        fact:               parent.fact
    }
}
