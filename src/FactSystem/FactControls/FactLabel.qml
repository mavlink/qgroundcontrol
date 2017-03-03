import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCLabel {
    property Fact fact: Fact { }
    text: fact.valueString
}
