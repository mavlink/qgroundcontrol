import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    objectName:     "toolbar_gpsIndicator"
    property bool showIndicator: _activeVehicle && _activeVehicle.gps.telemetryAvailable
}
