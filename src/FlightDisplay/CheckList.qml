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
    property int            unhealthySensors:       _activeVehicle ? _activeVehicle.sensorsUnhealthyBits : 0
    property bool           gpsLock:                _activeVehicle ? _activeVehicle.gps.lock.rawValue>=3 : 0
    property var            batPercentRemaining:    _activeVehicle ? _activeVehicle.battery.percentRemaining.value : 0
    property bool           audioMuted:             QGroundControl.settingsManager.appSettings.audioMuted.rawValue
    property ObjectModel    checkListItems:         _checkListItems
    property var            _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property int            _checkState:            _activeVehicle ? (_activeVehicle.armed ? 1 + (buttonActuators.state + buttonMotors.state + buttonMission.state + buttonSoundOutput.state) / 4 / 4 : 0) : 0 ; // Shows progress of checks inside the checklist - unlocks next check steps in groups

    // Connections
    onBatPercentRemainingChanged:   buttonBattery.updateItem();
    onGpsLockChanged:               buttonSensors.updateItem();
    onAudioMutedChanged:            buttonSoundOutput.updateItem();
    onUnhealthySensorsChanged:      updateVehicleDependentItems();

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleChanged: onActiveVehicleChanged();
    }
    Component.onCompleted: {
        if(QGroundControl.multiVehicleManager.vehicles.count > 0) {
            onActiveVehicleChanged();
        }
    }

    // Functions
    function updateVehicleDependentItems() {
        buttonSensors.updateItem();
        buttonBattery.updateItem();
        buttonRC.updateItem();
        buttonEstimator.updateItem();
    }
    function onActiveVehicleChanged() {
        buttonSoundOutput.updateItem();     // Just updated here for initialization once we connect to a vehicle
        updateVehicleDependentItems();
    }
    function resetNrClicks() {
        buttonHardware.resetNrClicks();
        buttonBattery.resetNrClicks();
        buttonRC.resetNrClicks();
        buttonActuators.resetNrClicks();
        buttonMotors.resetNrClicks();
        buttonMission.resetNrClicks();
        buttonSoundOutput.resetNrClicks();
        buttonPayload.resetNrClicks();
        buttonWeather.resetNrClicks();
        buttonFlightAreaFree.resetNrClicks();
    }

    // Check list item data
    ObjectModel {
        id: _checkListItems

        // Standard check list items (group 0) - Available from the start
        QGCCheckListItem {
            id: buttonHardware
            name: "Hardware"
            defaulttext: "Props mounted? Wings secured? Tail secured?"
        }
        QGCCheckListItem {
             id: buttonBattery
             name: "Battery"
             pendingtext: "Healthy & charged > 40%. Battery connector firmly plugged?"
             function updateItem() {
                 if (!_activeVehicle) {
                     state = statePassed
                 } else {
                     if (unhealthySensors & Vehicle.SysStatusSensorBattery) {
                         failuretext = qsTr("Not healthy. Check console.")
                         state = stateMajorIssue
                     } else if (batPercentRemaining < 40.0) {
                         failuretext = qsTr("Low (below 40%). Please recharge.")
                         state = stateMajorIssue
                     } else {
                         state = _nrClicked > 0 ? statePassed : statePending
                     }
                 }
             }
        }
        QGCCheckListItem {
             id: buttonSensors
             name: "Sensors"
             function updateItem() {
                 if (!_activeVehicle) {
                     state = statePassed
                 } else {
                     if(!(unhealthySensors & Vehicle.SysStatusSensor3dMag) &&
                        !(unhealthySensors & Vehicle.SysStatusSensor3dAccel) &&
                        !(unhealthySensors & Vehicle.SysStatusSensor3dGyro) &&
                        !(unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure) &&
                        !(unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure) &&
                        !(unhealthySensors & Vehicle.SysStatusSensorGPS)) {
                         if (!gpsLock) {
                             pendingtext = qsTr("Pending. Waiting for GPS lock.")
                             state = statePending
                         } else {
                             state = statePassed
                         }
                     } else {
                         if(unhealthySensors & Vehicle.SysStatusSensor3dMag)                        failuretext="Failure. Magnetometer issues. Check console.";
                         else if(unhealthySensors & Vehicle.SysStatusSensor3dAccel)                 failuretext="Failure. Accelerometer issues. Check console.";
                         else if(unhealthySensors & Vehicle.SysStatusSensor3dGyro)                  failuretext="Failure. Gyroscope issues. Check console.";
                         else if(unhealthySensors & Vehicle.SysStatusSensorAbsolutePressure)        failuretext="Failure. Barometer issues. Check console.";
                         else if(unhealthySensors & Vehicle.SysStatusSensorDifferentialPressure)    failuretext="Failure. Airspeed sensor issues. Check console.";
                         else if(unhealthySensors & Vehicle.SysStatusSensorGPS)                     failuretext="Failure. No valid or low quality GPS signal. Check console.";
                         state = stateMajorIssue
                     }
                 }
             }
        }
        QGCCheckListItem {
            id: buttonRC
            name: "Radio Control"
            pendingtext: "Receiving signal. Perform range test & confirm."
            failuretext: "No signal or invalid autopilot-RC config. Check RC and console."
            function updateItem() {
                if (!_activeVehicle) {
                    state = statePassed
                } else {
                    if (unhealthySensors & Vehicle.SysStatusSensorRCReceiver) {
                        state = stateMajorIssue
                    } else {
                        state = _nrClicked > 0 ? statePassed : statePending
                    }
                }
            }
        }
        QGCCheckListItem {
            id: buttonEstimator
            name: "Global position estimate"
            function updateItem() {
                if (!_activeVehicle) {
                    state = statePassed
                } else {
                    if (unhealthySensors & Vehicle.SysStatusSensorAHRS) {
                        state = stateMajorIssue
                    } else {
                        state = statePassed
                    }
                }
            }
        }

        // Check list item group 1 - Require arming
        QGCLabel {text:qsTr("<i>Please arm the vehicle here.</i>") ; opacity: 0.2+0.8*(QGroundControl.multiVehicleManager.vehicles.count > 0) ; anchors.horizontalCenter:buttonHardware.horizontalCenter ; anchors.topMargin:40 ; anchors.bottomMargin:40;}
        QGCCheckListItem {
           id: buttonActuators
           name: "Actuators"
           group: 1
           defaulttext: "Move all control surfaces. Did they work properly?"
        }
        QGCCheckListItem {
           id: buttonMotors
           name: "Motors"
           group: 1
           defaulttext: "Propellers free? Then throttle up gently. Working properly?"
        }
        QGCCheckListItem {
           id: buttonMission
           name: "Mission"
           group: 1
           defaulttext: "Please confirm mission is valid (waypoints valid, no terrain collision)."
        }
        QGCCheckListItem {
           id: buttonSoundOutput
           name: "Sound output"
           group: 1
           pendingtext: "QGC audio output enabled. System audio output enabled, too?"
           failuretext: "Failure, QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!"
           function updateItem() {
               if (!_activeVehicle) {
                   state = statePassed
               } else {
                   if (audioMuted) {
                       state = stateMajorIssue
                       _nrClicked = 0
                   } else {
                       state = _nrClicked > 0 ? statePassed : statePending
                   }
               }
           }
        }

        // Check list item group 2 - Final checks before launch
        QGCLabel {text:qsTr("<i>Last preparations before launch</i>") ; opacity : 0.2+0.8*(_checkState >= 2); anchors.horizontalCenter:buttonHardware.horizontalCenter}
        QGCCheckListItem {
           id: buttonPayload
           name: "Payload"
           group: 2
           defaulttext: "Configured and started?"
           pendingtext: "Payload lid closed?"
        }
        QGCCheckListItem {
           id: buttonWeather
           name: "Wind & weather"
           group: 2
           defaulttext: "OK for your platform?"
           pendingtext: "Launching into the wind?"
        }
        QGCCheckListItem {
           id: buttonFlightAreaFree
           name: "Flight area"
           group: 2
           defaulttext: "Launch area and path free of obstacles/people?"
        }
    } // Object Model
}
