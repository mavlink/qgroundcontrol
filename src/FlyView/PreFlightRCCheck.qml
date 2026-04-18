import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    name:                   qsTr("Radio Control")
    manualText:             qsTr("Receiving signal. Perform range test & confirm.")
    telemetryTextFailure:   qsTr("No signal or invalid autopilot-RC config. Check RC and console.")
    telemetryFailure:       false//_unhealthySensors & MAVLinkEnums.MAV_SYS_STATUS_SENSOR_RC_RECEIVER

    property int _unhealthySensors: globals.activeVehicle ? globals.activeVehicle.sensorsUnhealthyBits : 0
}
