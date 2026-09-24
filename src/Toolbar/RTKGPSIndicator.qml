import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    // Receivers can always be connected over TCP, even in builds without serial support.
    property bool showIndicator: !_activeVehicle
}
