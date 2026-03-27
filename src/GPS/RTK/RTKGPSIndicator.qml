import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    property bool showIndicator: !_activeVehicle && _rtkConnected
}
