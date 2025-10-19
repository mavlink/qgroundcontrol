/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl
import QGroundControl.FlightDisplay

ToolStripAction {
    id:         action
    text:       qsTr("Gripper")
    iconSource: "/res/Gripper.svg"          
    visible:    _gripperAvailable

    property var   _activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle 
    property bool  _gripperAvailable:   _activeVehicle ? _activeVehicle.hasGripper : false

    dropPanelComponent: Component {
        FlyViewGripperDropPanel { 
        }
    }
}
