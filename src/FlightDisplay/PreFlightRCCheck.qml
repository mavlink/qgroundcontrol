/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Vehicle

PreFlightCheckButton {
    name:                   qsTr("Radio Control")
    manualText:             qsTr("Receiving signal. Perform range test & confirm.")
    telemetryTextFailure:   qsTr("No signal or invalid autopilot-RC config. Check RC and console.")
    telemetryFailure:       false//_unhealthySensors & Vehicle.SysStatusSensorRCReceiver

    property int _unhealthySensors: globals.activeVehicle ? globals.activeVehicle.sensorsUnhealthyBits : 0
}
