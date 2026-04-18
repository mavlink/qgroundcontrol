import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    name:               qsTr("Sensors")
    telemetryFailure:   _unhealthySensors & _allCheckedSensors

    property int    _unhealthySensors:  globals.activeVehicle ? globals.activeVehicle.sensorsUnhealthyBits : 1
    property int    _allCheckedSensors: MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_MAG |
                                        MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_ACCEL |
                                        MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_GYRO |
                                        MAVLinkEnums.MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE |
                                        MAVLinkEnums.MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE |
                                        MAVLinkEnums.MAV_SYS_STATUS_SENSOR_GPS |
                                        MAVLinkEnums.MAV_SYS_STATUS_AHRS

    on_UnhealthySensorsChanged: updateTelemetryTextFailure()

    Component.onCompleted: updateTelemetryTextFailure()

    function updateTelemetryTextFailure() {
        if(_unhealthySensors & _allCheckedSensors) {
            if (_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_MAG)                       telemetryTextFailure = qsTr("Failure. Magnetometer issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_ACCEL)                 telemetryTextFailure = qsTr("Failure. Accelerometer issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_GYRO)                  telemetryTextFailure = qsTr("Failure. Gyroscope issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE)        telemetryTextFailure = qsTr("Failure. Barometer issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE)    telemetryTextFailure = qsTr("Failure. Airspeed sensor issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_AHRS)                            telemetryTextFailure = qsTr("Failure. AHRS issues. Check console.")
            else if(_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_GPS)                      telemetryTextFailure = qsTr("Failure. GPS issues. Check console.")
        }
    }
}
