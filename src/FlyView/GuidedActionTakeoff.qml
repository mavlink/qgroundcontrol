import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    objectName: "flyToolStrip_takeoffButton"
    text:       _guidedController.takeoffTitle
    iconSource: "/res/takeoff.svg"
    visible:    !_guidedController.takeoffLandSlotShowsLand
    enabled:    _guidedController.showTakeoff && _guidedController.takeoffReady
    actionID:   _guidedController.actionTakeoff
}
