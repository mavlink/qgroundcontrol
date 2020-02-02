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

// This class stores the data and functions of the check list but NOT the GUI (which is handled somewhere else).
PreFlightCheckButton {
    name:                           qsTr("Battery")
    manualText:                     qsTr("Battery connector firmly plugged?")
    telemetryFailure:               _batLow
    telemetryTextFailure:           allowTelemetryFailureOverride ?
                                        qsTr("Warning - Battery charge below %1%.").arg(failurePercent) :
                                        qsTr("Battery charge below %1%. Please recharge.").arg(failurePercent)
    allowTelemetryFailureOverride:  allowFailurePercentOverride

    property int    failurePercent:                 40
    property bool   allowFailurePercentOverride:    false
    property var    _batPercentRemaining:           activeVehicle ? activeVehicle.battery.percentRemaining.value : 0
    property bool   _batLow:                        _batPercentRemaining < failurePercent
}
