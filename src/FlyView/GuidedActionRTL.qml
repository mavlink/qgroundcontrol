import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    text:       _guidedController.rtlTitle
    iconSource: "/res/rtl.svg"
    visible:    !_activeVehicle || _activeVehicle.rtlFlightMode !== ""
    enabled:    _guidedController.showRTL
    actionID:   _guidedController.actionRTL

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
}
