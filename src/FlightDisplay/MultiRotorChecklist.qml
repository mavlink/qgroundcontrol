/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQml.Models

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Vehicle

Item {
    property var model: listModel
    PreFlightCheckModel {
        id:     listModel
        PreFlightCheckGroup {
            name: qsTr("Multirotor Initial Checks")

            PreFlightCheckButton {
                name:           qsTr("Hardware")
                manualText:     globals.activeVehicle ? (globals.activeVehicle.checkListItem1 ? "" :  qsTr("Props mounted and secured?")) : qsTr("Props mounted and secured?")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem1 ? false : true) : true
                telemetryTextFailure: qsTr("Props mounted and secured?")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem1 = true
                    }
                }
            }

            PreFlightBatteryCheck {
                failurePercent:                 40
                allowFailurePercentOverride:    false
            }

            PreFlightSensorsHealthCheck {
            }

            PreFlightGPSCheck {
                failureSatCount:        9
                allowOverrideSatCount:  true
            }

            PreFlightRCCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Please arm the vehicle here")

            PreFlightCheckButton {
                name:            qsTr("Motors")
                manualText:      globals.activeVehicle ? (globals.activeVehicle.checkListItem2 ? "" :  qsTr("Propellers free? Then throttle up gently. Working properly?")) : qsTr("Propellers free? Then throttle up gently. Working properly?")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem2 ? false : true) : true
                telemetryTextFailure: qsTr("Propellers free? Then throttle up gently. Working properly?")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem2 = true
                    }
                }
            }

            PreFlightCheckButton {
                name:           qsTr("Mission")
                manualText:     globals.activeVehicle ? (globals.activeVehicle.checkListItem3 ? "" :  qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")) : qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem3 ? false : true) : true
                telemetryTextFailure: qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem3 = true
                    }
                }
            }

            PreFlightSoundCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Last preparations before launch")

            // Check list item group 2 - Final checks before launch
            PreFlightCheckButton {
                name:           qsTr("Payload")
                manualText:     globals.activeVehicle ? (globals.activeVehicle.checkListItem4 ? "" :  qsTr("Configured and started? Payload lid closed?")) : qsTr("Configured and started? Payload lid closed?")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem4 ? false : true) : true
                telemetryTextFailure: qsTr("Configured and started? Payload lid closed?")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem4 = true
                    }
                }
            }

            PreFlightCheckButton {
                name:           qsTr("Wind & weather")
                manualText:     globals.activeVehicle ? (globals.activeVehicle.checkListItem5 ? "" :  qsTr("OK for your platform?")) : qsTr("OK for your platform?")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem5 ? false : true) : true
                telemetryTextFailure: qsTr("OK for your platform?")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem5 = true
                    }
                }
            }

            PreFlightCheckButton {
                name:           qsTr("Flight area")
                manualText:      globals.activeVehicle ? (globals.activeVehicle.checkListItem6 ? "" :  qsTr("Launch area and path free of obstacles/people?")) : qsTr("Launch area and path free of obstacles/people?")
                telemetryFailure: globals.activeVehicle ? (globals.activeVehicle.checkListItem6 ? false : true) : true
                telemetryTextFailure: qsTr("Launch area and path free of obstacles/people?")
                allowTelemetryFailureOverride: true
                onClicked: {
                    if (manualText !== "") {
                        // User is confirming a manual check
                        _manualState = (_manualState === _statePassed) ? _statePending : _statePassed
                        globals.activeVehicle.checkListItem6 = true
                    }
                }
            }
        }
    }
}

