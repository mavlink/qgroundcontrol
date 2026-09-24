import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    // Receivers can always be connected over TCP, even in builds without serial support. The vehicle
    // indicator replaces this one only while the vehicle reports GPS telemetry.
    property bool showIndicator: !_activeVehicle || !_activeVehicle.gps || !_activeVehicle.gps.telemetryAvailable
}
