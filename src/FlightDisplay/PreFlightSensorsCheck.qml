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
    name: qsTr("Sensors")

    property int failureSatCount: -1    ///< -1 indicates no sat count check

    property int    _unhealthySensors:  _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property bool   _gpsLock:           _activeVehicle ? _activeVehicle.gps.lock.rawValue >= 3 : 0
    property bool   _satCount:          _activeVehicle ? _activeVehicle.gps.count : 0
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property int    _allCheckedSensors: Vehicle.SysStatusSensor3dMag |
                                        Vehicle.SysStatusSensor3dAccel |
                                        Vehicle.SysStatusSensor3dGyro |
                                        Vehicle.SysStatusSensorAbsolutePressure |
                                        Vehicle.SysStatusSensorDifferentialPressure |
                                        Vehicle.SysStatusSensorGPS


    on_GpsLockChanged:          updateItem()
    on_SatCountChanged:         updateItem()
    on_UnhealthySensorsChanged: updateItem()
    on_ActiveVehicleChanged:    updateItem()

    Component.onCompleted: updateItem()

    function updateItem() {
        if (!_activeVehicle) {
            state = stateNotChecked
        } else {
            if(!(_unhealthySensors & _allCheckedSensors)) {
                if (!_gpsLock) {
                    pendingText = qsTr("Pending. Waiting for GPS lock.")
                    state = statePending
                } else if (failureSatCount !== -1 && _satCount <= failureSatCount) {
                    pendingText = qsTr("Pending. Waiting for Sat Count > %1.").arg(failureSatCount)
                    state = statePending
                } else {
                    state = statePassed
                }
            } else {
                if (_unhealthySensors & Vehicle.SysStatusSensor3dMag)                       failureText=qsTr("Failure. Magnetometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensor3dAccel)                 failureText=qsTr("Failure. Accelerometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensor3dGyro)                  failureText=qsTr("Failure. Gyroscope issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure)        failureText=qsTr("Failure. Barometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure)    failureText=qsTr("Failure. Airspeed sensor issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorGPS)                     failureText=qsTr("Failure. No valid or low quality GPS signal. Check console.")
                state = stateMajorIssue
            }
        }
    }
}
