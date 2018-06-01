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

// This class stores the data and functions of the check list but NOT the GUI (which is handled somewhere else).
PreFlightCheckButton {
    name:                   qsTr("Battery")
    manualText:             qsTr("Healthy & charged > %1. Battery connector firmly plugged?").arg(failureVoltage)
    telemetryTextFailure:   _batUnHealthy ?
                                qsTr("Not healthy. Check console.") :
                                ("Low (below %1). Please recharge.").arg(failureVoltage)

    property int failureVoltage: 40

    property var _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property int _unhealthySensors:     _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property var _batPercentRemaining:  _activeVehicle ? _activeVehicle.battery.percentRemaining.value : 0
    property bool _batUnHealthy:        _unhealthySensors & Vehicle.SysStatusSensorBattery
    property bool _batLow:              _batPercentRemaining < failureVoltage
}
