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
    name:           qsTr("Battery")
    pendingText:    qsTr("Healthy & charged > %1. Battery connector firmly plugged?").arg(failureVoltage)

    property int failureVoltage: 40

    property int _unhealthySensors:     _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property var _batPercentRemaining:  _activeVehicle ? _activeVehicle.battery.percentRemaining.value : 0
    property var _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle

    on_BatPercentRemainingChanged:  updateItem()
    on_UnhealthySensorsChanged:     updateItem()
    on_ActiveVehicleChanged:        updateItem()

    Component.onCompleted: updateItem()

    function updateItem() {
        if (!_activeVehicle) {
            state = stateNotChecked
        } else {
            if (_unhealthySensors & Vehicle.SysStatusSensorBattery) {
                failureText = qsTr("Not healthy. Check console.")
                state = stateMajorIssue
            } else if (_batPercentRemaining < failureVoltage) {
                failureText = qsTr("Low (below %1). Please recharge.").arg(failureVoltage)
                state = stateMajorIssue
            } else {
                state = _nrClicked > 0 ? statePassed : statePending
            }
        }
    }
}
