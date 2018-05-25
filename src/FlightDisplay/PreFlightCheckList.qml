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

// This class stores the data and functions of the check list but NOT the GUI (which is handled somewhere else).
Item {
    // Properties
    property ObjectModel    checkListItems:         _checkListItems
    property var            _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property int            _checkState:            _activeVehicle ? (_activeVehicle.armed ? 1 + (buttonActuators.state + buttonMotors.state + buttonMission.state + buttonSoundOutput.state) / 4 / 4 : 0) : 0 ; // Shows progress of checks inside the checklist - unlocks next check steps in groups

    function reset() {
        buttonHardware.reset();
        buttonBattery.reset();
        buttonRC.reset();
        buttonActuators.reset();
        buttonMotors.reset();
        buttonMission.reset();
        buttonSoundOutput.reset();
        buttonPayload.reset();
        buttonWeather.reset();
        buttonFlightAreaFree.reset();
    }

    // Check list item data
    ObjectModel {
        id: _checkListItems

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

        // Check list item group 1 - Require arming
        QGCLabel {text:qsTr("<i>Please arm the vehicle here.</i>") ; opacity: 0.2+0.8*(QGroundControl.multiVehicleManager.vehicles.count > 0) ; anchors.horizontalCenter:buttonHardware.horizontalCenter ; anchors.topMargin:40 ; anchors.bottomMargin:40;}
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

        // Check list item group 2 - Final checks before launch
        QGCLabel {text:qsTr("<i>Last preparations before launch</i>") ; opacity : 0.2+0.8*(_checkState >= 2); anchors.horizontalCenter:buttonHardware.horizontalCenter}
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
    } // Object Model
}
