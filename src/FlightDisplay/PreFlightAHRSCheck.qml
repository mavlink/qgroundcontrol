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
    name:                   qsTr("Global position estimate")
    telemetryTextFailure:   qsTr("AHRS Unhealthy. Check console.")
    telemetryFailure:       _unhealthySensors & Vehicle.SysStatusSensorAHRS

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property int _unhealthySensors: _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
}
