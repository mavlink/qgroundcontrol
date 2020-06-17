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
    name:                   qsTr("Radio Control")
    manualText:             qsTr("Receiving signal. Perform range test & confirm.")
    telemetryTextFailure:   qsTr("No signal or invalid autopilot-RC config. Check RC and console.")
    telemetryFailure:       _unhealthySensors & Vehicle.SysStatusSensorRCReceiver

    property int _unhealthySensors: activeVehicle ? activeVehicle.sensorsUnhealthyBits : 0
}
