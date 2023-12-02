import QtQuick
import QtQuick.Controls
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.Controls

QGCLabel {
    property Fact fact: Fact { }
    text: fact.valueString
}
