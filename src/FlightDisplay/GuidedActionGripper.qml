/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.FlightDisplay 1.0
import QGroundControl 1.0

GuidedToolStripAction {
    property var   activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle 
    property bool  _initialConnectComplete: activeVehicle ? activeVehicle.initialConnectComplete : false
    property bool  _grip_enable:            _initialConnectComplete ? activeVehicle.hasGripper : false
    property bool  _isVehicleArmed:         _initialConnectComplete ? activeVehicle.armed : false

    text:       "Gripper"
    iconSource: "/res/Gripper.svg"          
    visible:    !_isVehicleArmed && _grip_enable   // in this way if the pilot it's on the ground can release the cargo without actions tool
    enabled:    _grip_enable
    actionID:   _guidedController.actionGripper
}
