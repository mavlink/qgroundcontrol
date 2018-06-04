/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.3
import QtQml.Models                 2.1

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

ObjectModel {
    PreFlightCheckGroup {
        name: qsTr("Initial checks")

        // Standard check list items (group 0) - Available from the start
        PreFlightCheckButton {
            id:             buttonHardware
            name:           qsTr("Hardware")
            manualText:     qsTr("Props mounted? Wings secured? Tail secured?")
        }
        PreFlightBatteryCheck {
            id:                buttonBattery
            failureVoltage:    40
        }
        PreFlightSensorsCheck {
            id: buttonSensors
        }
        PreFlightRCCheck {
            id: buttonRC
        }
        PreFlightAHRSCheck {
            id: buttonEstimator
        }
    }

    PreFlightCheckGroup {
        name: qsTr("Please arm the vehicle here")

        PreFlightCheckButton {
            id:              buttonActuators
            name:            qsTr("Actuators")
            group:           1
            manualText:      qsTr("Move all control surfaces. Did they work properly?")
        }
        PreFlightCheckButton {
            id:              buttonMotors
            name:            qsTr("Motors")
            group:           1
            manualText:      qsTr("Propellers free? Then throttle up gently. Working properly?")
        }
        PreFlightCheckButton {
            id:          buttonMission
            name:        qsTr("Mission")
            group:       1
            manualText:  qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")
        }
        PreFlightSoundCheck {
            id:      buttonSoundOutput
            group:   1
        }
    }

    PreFlightCheckGroup {
        name: qsTr("Last preparations before launch")

        // Check list item group 2 - Final checks before launch
        PreFlightCheckButton {
            id:          buttonPayload
            name:        qsTr("Payload")
            group:       2
            manualText:  qsTr("Configured and started? Payload lid closed?")
        }
        PreFlightCheckButton {
            id:          buttonWeather
            name:        "Wind & weather"
            group:       2
            manualText:  qsTr("OK for your platform? Lauching into the wind?")
        }
        PreFlightCheckButton {
            id:          buttonFlightAreaFree
            name:        qsTr("Flight area")
            group:       2
            manualText:  qsTr("Launch area and path free of obstacles/people?")
        }
    }
} // Object Model
