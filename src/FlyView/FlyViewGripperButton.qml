import QGroundControl
import QGroundControl.FlyView

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
