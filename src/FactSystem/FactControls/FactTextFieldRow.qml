import QtQuick
import QtQuick.Layouts

import QGroundControl.FactSystem
import QGroundControl.Controls

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
