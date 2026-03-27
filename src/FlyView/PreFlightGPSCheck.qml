import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    name:                           qsTr("GPS")
    telemetryFailure:               _3dLockFailure || _satCountFailure || _hdopFailure
    telemetryTextFailure:           _3dLockFailure ?
                                        qsTr("Waiting for 3D lock.") :
                                        (_hdopFailure ? _hdopFailureText :
                                        (_satCountFailure ? _satCountFailureText : ""))
    allowTelemetryFailureOverride:  !_3dLockFailure && ((_satCountFailure && allowOverrideSatCount) || _hdopFailure)

    property bool   allowOverrideSatCount:  false   ///< true: sat count above failureSatCount reguired to pass, false: user can click past satCount <= failureSetCount
    property int    failureSatCount:        -1      ///< -1 indicates no sat count check
    property real   failureHdop:            -1      ///< -1 indicates no HDOP check

    property bool   _3dLock:                globals.activeVehicle ? globals.activeVehicle.gps.lock.rawValue >= VehicleGPSFactGroup.Fix3D : false
    property int    _satCount:              globals.activeVehicle ? globals.activeVehicle.gps.count.rawValue : 0
    property real   _hdop:                  globals.activeVehicle && !isNaN(globals.activeVehicle.gps.hdop.rawValue) ? globals.activeVehicle.gps.hdop.rawValue : -1
    property bool   _3dLockFailure:         !_3dLock
    property bool   _satCountFailure:       failureSatCount !== -1 && _satCount <= failureSatCount
    property bool   _hdopFailure:           failureHdop > 0 && _hdop > 0 && _hdop > failureHdop
    property string _satCountFailureText:   allowOverrideSatCount ? qsTr("Warning - Sat count below %1.").arg(failureSatCount + 1) : qsTr("Waiting for sat count above %1.").arg(failureSatCount)
    property string _hdopFailureText:       qsTr("Warning - HDOP %1 exceeds limit %2.").arg(_hdop.toFixed(1)).arg(failureHdop.toFixed(1))
}
