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

    property int    _unhealthySensors:  _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property bool   _gpsLock:           _activeVehicle ? _activeVehicle.gps.lock.rawValue>=3 : 0
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

    on_GpsLockChanged:          updateItem()
    on_UnhealthySensorsChanged: updateItem()
    on_ActiveVehicleChanged:    updateItem()

    Component.onCompleted: updateItem()

    function updateItem() {
        if (!_activeVehicle) {
            state = stateNotChecked
        } else {
            if(!(_unhealthySensors & Vehicle.SysStatusSensor3dMag) &&
                    !(_unhealthySensors & Vehicle.SysStatusSensor3dAccel) &&
                    !(_unhealthySensors & Vehicle.SysStatusSensor3dGyro) &&
                    !(_unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure) &&
                    !(_unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure) &&
                    !(_unhealthySensors & Vehicle.SysStatusSensorGPS)) {
                if (!_gpsLock) {
                    pendingtext = qsTr("Pending. Waiting for GPS lock.")
                    state = statePending
                } else {
                    state = statePassed
                }
            } else {
                if (_unhealthySensors & Vehicle.SysStatusSensor3dMag)                       failuretext=qsTr("Failure. Magnetometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensor3dAccel)                 failuretext=qsTr("Failure. Accelerometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensor3dGyro)                  failuretext=qsTr("Failure. Gyroscope issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure)        failuretext=qsTr("Failure. Barometer issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure)    failuretext=qsTr("Failure. Airspeed sensor issues. Check console.")
                else if(_unhealthySensors & Vehicle.SysStatusSensorGPS)                     failuretext=qsTr("Failure. No valid or low quality GPS signal. Check console.")
                state = stateMajorIssue
            }
        }
    }
}
