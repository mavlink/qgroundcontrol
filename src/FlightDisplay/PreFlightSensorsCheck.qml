/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0

PreFlightCheckButton {
    name:               qsTr("Sensors")
    telemetryFailure:   (_unhealthySensors & _allCheckedSensors) || !_gpsLock || _satCountFailure

    property int failureSatCount: -1    ///< -1 indicates no sat count check

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property int    _unhealthySensors:  _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property bool   _gpsLock:           _activeVehicle ? _activeVehicle.gps.lock.rawValue >= 3 : 0
    property int    _satCount:          _activeVehicle ? _activeVehicle.gps.count.rawValue : 0
    property bool   _satCountFailure:   failureSatCount !== -1 && _satCount <= failureSatCount
    property int    _allCheckedSensors: Vehicle.SysStatusSensor3dMag |
                                        Vehicle.SysStatusSensor3dAccel |
                                        Vehicle.SysStatusSensor3dGyro |
                                        Vehicle.SysStatusSensorAbsolutePressure |
                                        Vehicle.SysStatusSensorDifferentialPressure |
                                        Vehicle.SysStatusSensorGPS

    on_GpsLockChanged:          updateTelemetryTextFailure()
    on_SatCountFailureChanged:  updateTelemetryTextFailure()
    on_UnhealthySensorsChanged: updateTelemetryTextFailure()

    Component.onCompleted: updateTelemetryTextFailure()

    function updateTelemetryTextFailure() {
        if(_unhealthySensors & _allCheckedSensors) {
            if (_unhealthySensors & Vehicle.SysStatusSensor3dMag)                       telemetryTextFailure = qsTr("Failure. Magnetometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensor3dAccel)                 telemetryTextFailure = qsTr("Failure. Accelerometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensor3dGyro)                  telemetryTextFailure = qsTr("Failure. Gyroscope issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure)        telemetryTextFailure = qsTr("Failure. Barometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure)    telemetryTextFailure = qsTr("Failure. Airspeed sensor issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorGPS)                     telemetryTextFailure = qsTr("Failure. No valid or low quality GPS signal. Check console.")
        } else if (!_gpsLock) {
            telemetryTextFailure = qsTr("Pending. Waiting for GPS lock.")
        } else if (_satCountFailure) {
            telemetryTextFailure = qsTr("Pending. Waiting for Sat Count > %1.").arg(failureSatCount)
        }
    }
}
