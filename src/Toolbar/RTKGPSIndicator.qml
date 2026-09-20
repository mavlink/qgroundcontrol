import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    property bool showIndicator: !_activeVehicle && (_rtkConnected || _serialSupported)

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rtkConnected:  QGroundControl.gpsRtk.connected.value
    readonly property bool _serialSupported: QGroundControl.gpsManager
                                            && QGroundControl.gpsManager.gpsRtk
                                            && QGroundControl.gpsManager.gpsRtk.serialSupported
}
