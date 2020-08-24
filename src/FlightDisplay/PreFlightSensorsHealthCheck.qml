/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    telemetryFailure:   _unhealthySensors & _allCheckedSensors

    property int    _unhealthySensors:  globals.activeVehicle ? globals.activeVehicle.sensorsUnhealthyBits : 1
    property int    _allCheckedSensors: Vehicle.SysStatusSensor3dMag |
                                        Vehicle.SysStatusSensor3dAccel |
                                        Vehicle.SysStatusSensor3dGyro |
                                        Vehicle.SysStatusSensorAbsolutePressure |
                                        Vehicle.SysStatusSensorDifferentialPressure |
                                        Vehicle.SysStatusSensorGPS |
                                        Vehicle.SysStatusSensorAHRS

    on_UnhealthySensorsChanged: updateTelemetryTextFailure()

    Component.onCompleted: updateTelemetryTextFailure()

    function updateTelemetryTextFailure() {
        if(_unhealthySensors & _allCheckedSensors) {
            if (_unhealthySensors & Vehicle.SysStatusSensor3dMag)                       telemetryTextFailure = qsTr("Failure. Magnetometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensor3dAccel)                 telemetryTextFailure = qsTr("Failure. Accelerometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensor3dGyro)                  telemetryTextFailure = qsTr("Failure. Gyroscope issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure)        telemetryTextFailure = qsTr("Failure. Barometer issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure)    telemetryTextFailure = qsTr("Failure. Airspeed sensor issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorAHRS)                    telemetryTextFailure = qsTr("Failure. AHRS issues. Check console.")
            else if(_unhealthySensors & Vehicle.SysStatusSensorGPS)                     telemetryTextFailure = qsTr("Failure. GPS issues. Check console.")
        }
    }
}
