/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Layouts          1.2

import QGroundControl                           1.0
import QGroundControl.ScreenTools               1.0
import QGroundControl.Controls                  1.0
import QGroundControl.Palette                   1.0
import QGroundControl.Vehicle                   1.0
import QGroundControl.FlightMap                 1.0

/// This provides the smarts behind the guided mode commands, minus the user interface. This way you can change UI
/// without affecting the underlying functionality.
Item {
    id: _root

    property var missionController

    signal showConfirmAction(string title, string message, int action, var actionData)

    readonly property string emergencyStopTitle:    qsTr("Emergency Stop")
    readonly property string armTitle:              qsTr("Arm")
    readonly property string disarmTitle:           qsTr("Disarm")
    readonly property string rtlTitle:              qsTr("RTL")
    readonly property string takeoffTitle:          qsTr("Takeoff")
    readonly property string landTitle:             qsTr("Land")
    readonly property string startMissionTitle:     qsTr("Start Mission")
    readonly property string resumeMissionTitle:    qsTr("Resume Mission")
    readonly property string pauseTitle:            qsTr("Pause")
    readonly property string changeAltTitle:        qsTr("Change Altitude")
    readonly property string orbitTitle:            qsTr("Orbit")
    readonly property string landAbortTitle:        qsTr("Land Abort")
    readonly property string setWaypointTitle:      qsTr("Set Waypoint")
    readonly property string gotoTitle:             qsTr("Goto Location")

    readonly property string armMessage:                qsTr("Arm the vehicle.")
    readonly property string disarmMessage:             qsTr("Disarm the vehicle")
    readonly property string emergencyStopMessage:      qsTr("WARNING: This still stop all motors. If vehicle is currently in air it will crash.")
    readonly property string takeoffMessage:            qsTr("Takeoff from ground and hold position.")
    readonly property string startMissionMessage:       qsTr("Start the mission which is currently displayed above. If the vehicle is on the ground it will takeoff.")
             property string resumeMissionMessage:      qsTr("Resume the mission which is displayed above. This will re-generate the mission from waypoint %1, takeoff and continue the mission.").arg(_resumeMissionIndex)
    readonly property string resumeMissionReadyMessage: qsTr("Review the modified mission above. Confirm if you want to takeoff and begin mission.")
    readonly property string landMessage:               qsTr("Land the vehicle at the current position.")
    readonly property string rtlMessage:                qsTr("Return to the home position of the vehicle.")
    readonly property string changeAltMessage:          qsTr("Change the altitude of the vehicle up or down.")
    readonly property string gotoMessage:               qsTr("Move the vehicle to the location clicked on the map.")
             property string setWaypointMessage:        qsTr("Adjust current waypoint to %1.").arg(_actionData)
    readonly property string orbitMessage:              qsTr("Orbit the vehicle around the current location.")
    readonly property string landAbortMessage:          qsTr("Abort the landing sequence.")
    readonly property string pauseMessage:              qsTr("Pause the vehicle at it's current position.")

    readonly property int actionRTL:                1
    readonly property int actionLand:               2
    readonly property int actionTakeoff:            3
    readonly property int actionArm:                4
    readonly property int actionDisarm:             5
    readonly property int actionEmergencyStop:      6
    readonly property int actionChangeAlt:          7
    readonly property int actionGoto:               8
    readonly property int actionSetWaypoint:        9
    readonly property int actionOrbit:              10
    readonly property int actionLandAbort:          11
    readonly property int actionStartMission:       12
    readonly property int actionResumeMission:      13
    readonly property int actionResumeMissionReady: 14
    readonly property int actionPause:              15

    property bool showEmergenyStop:     !_hideEmergenyStop && _activeVehicle && _vehicleArmed && _activeVehicle.flying
    property bool showDisarm:           _activeVehicle && _vehicleArmed && !_activeVehicle.flying
    property bool showRTL:              _activeVehicle && _vehicleArmed && _activeVehicle.guidedModeSupported && _activeVehicle.flying && !_vehicleInRTLMode
    property bool showTakeoff:          _activeVehicle && _activeVehicle.guidedModeSupported && !_activeVehicle.flying  && !_activeVehicle.fixedWing
    property bool showLand:             _activeVehicle && _activeVehicle.guidedModeSupported && _vehicleArmed && !_activeVehicle.fixedWing && !_vehicleInLandMode
    property bool showStartMission:     _activeVehicle && _missionAvailable && !_missionActive
    property bool showResumeMission:    _activeVehicle && !_activeVehicle.flying && _missionAvailable && _resumeMissionIndex > 0
    property bool showPause:            _activeVehicle && _vehicleArmed && _activeVehicle.pauseVehicleSupported && _activeVehicle.flying && !_vehiclePaused
    property bool showChangeAlt:        (_activeVehicle && _activeVehicle.flying) && _activeVehicle.guidedModeSupported && _vehicleArmed && !_missionActive
    property bool showOrbit:            !_hideOrbit && _activeVehicle && _activeVehicle.flying && _activeVehicle.orbitModeSupported && _vehicleArmed && !_missionActive
    property bool showLandAbort:        _activeVehicle && _activeVehicle.flying && _activeVehicle.fixedWing
    property bool showGotoLocation:     _activeVehicle && _activeVehicle.guidedMode && _activeVehicle.flying

    onShowStartMissionChanged: console.log(showStartMission, _activeVehicle, _missionAvailable, _missionActive, _vehicleArmed, _vehicleInLandMode, _vehicleInRTLMode)

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property string _flightMode:            _activeVehicle ? _activeVehicle.flightMode : ""
    property bool   _missionAvailable:      missionController.containsItems
    property bool   _missionActive:         _activeVehicle ? _vehicleArmed && (_vehicleInLandMode || _vehicleInRTLMode || _vehicleInMissionMode) : false
    property bool   _vehicleArmed:          _activeVehicle ? _activeVehicle.armed  : false
    property bool   _vehiclePaused:         false
    property bool   _vehicleInMissionMode:  false
    property bool   _vehicleInRTLMode:      false
    property bool   _vehicleInLandMode:     false
    property int    _resumeMissionIndex:    missionController.resumeMissionIndex
    property bool   _hideEmergenyStop:      !QGroundControl.corePlugin.options.guidedBarShowEmergencyStop
    property bool   _hideOrbit:             !QGroundControl.corePlugin.options.guidedBarShowOrbit
    property var    _actionData

    on_FlightModeChanged: {
        _vehiclePaused =        _flightMode === _activeVehicle.pauseFlightMode
        _vehicleInRTLMode =     _flightMode === _activeVehicle.rtlFlightMode
        _vehicleInLandMode =    _flightMode === _activeVehicle.landFlightMode
        _vehicleInMissionMode = _flightMode === _activeVehicle.missionFlightMode // Must be last to get correct signalling for showStartMission popups
    }

    // Called when an action is about to be executed in order to confirm
    function confirmAction(actionCode, actionData) {
        var title
        var message
        _actionData = actionData
        switch (actionCode) {
        case actionArm:
            if (_activeVehicle.flying) {
                return
            }
            title = armTitle
            message = armMessage
            break;
        case actionDisarm:
            if (_activeVehicle.flying) {
                return
            }
            title = disarmTitle
            message = disarmMessage
            break;
        case actionEmergencyStop:
            title = emergencyStopTitle
            message = emergencyStopMessage
            break;
        case actionTakeoff:
            title = takeoffTitle
            message = takeoffMessage
            break;
        case actionStartMission:
            title = startMissionTitle
            message = startMissionMessage
            break;
        case actionResumeMission:
            title = resumeMissionTitle
            message = resumeMissionMessage
            break;
        case actionResumeMissionReady:
            title = resumeMissionTitle
            message = resumeMissionReadyMessage
            break;
        case actionLand:
            title = landTitle
            message = landMessage
            break;
        case actionRTL:
            title = rtlTitle
            message = rtlMessage
            break;
        case actionChangeAlt:
            title = changeAltTitle
            message = changeAltMessage
            break;
        case actionGoto:
            title = gotoTitle
            message = gotoMessage
            break;
        case actionSetWaypoint:
            title = setWaypointTitle
            message = setWaypointMessage
            break;
        case actionOrbit:
            title = orbitTitle
            message = orbitMessage
            break;
        case actionLandAbort:
            title = landAbortTitle
            message = landAbortMessage
            break;
        case actionPause:
            title = pauseTitle
            message = pauseMessage
            break;
        default:
            console.warn("Unknown actionCode", actionCode)
            return
        }
        showConfirmAction(title, message, actionCode, actionData)
    }

    // Executes the specified action
    function executeAction(actionCode, actionData) {
        switch (actionCode) {
        case actionRTL:
            _activeVehicle.guidedModeRTL()
            break
        case actionLand:
            _activeVehicle.guidedModeLand()
            break
        case actionTakeoff:
            _activeVehicle.guidedModeTakeoff()
            break
        case actionResumeMission:
            missionController.resumeMission(missionController.resumeMissionIndex)
            break
        case actionResumeMissionReady:
            _activeVehicle.startMission()
            break
        case actionStartMission:
            _activeVehicle.startMission()
            break
        case actionArm:
            _vehicleArmed = true
            break
        case actionDisarm:
            _vehicleArmed = false
            break
        case actionEmergencyStop:
            _activeVehicle.emergencyStop()
            break
        case actionChangeAlt:
            _activeVehicle.guidedModeChangeAltitude(actionData)
            break
        case actionGoto:
            _activeVehicle.guidedModeGotoLocation(actionData)
            break
        case actionSetWaypoint:
            _activeVehicle.setCurrentMissionSequence(actionData)
            break
        case actionOrbit:
            _activeVehicle.guidedModeOrbit()
            break
        case actionLandAbort:
            _activeVehicle.abortLanding(50)     // hardcoded value for climbOutAltitude that is currently ignored
            break
        case actionPause:
            _activeVehicle.pauseVehicle()
            break
        default:
            console.warn(qsTr("Internal error: unknown actionCode"), actionCode)
            break
        }
    }
}
