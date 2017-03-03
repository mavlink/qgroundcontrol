import QtQuick          2.7
import QtQuick.Controls 2.1

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FactSystem    1.0

QGCLabel {
    width:      availableWidth
    wrapMode:   Text.WordWrap
    text:       qsTr("This vehicle does not support GeoFence.")

    //property var contoller - controller - Must be passed in from Loader
    //property real availableWidth - Available width for control - Must be passed in from Loader
}
