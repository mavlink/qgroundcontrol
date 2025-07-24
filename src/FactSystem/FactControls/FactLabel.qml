import QtQuick
import QtQuick.Controls

import QGroundControl


import QGroundControl.Controls

QGCLabel {
    property bool showUnits:    true
    property Fact fact:         Fact { }

    text: fact.valueString + (showUnits ? " " + fact.units : "")
}
