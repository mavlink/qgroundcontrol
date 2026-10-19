import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    objectName: "flyToolStrip_landButton"
    text:       _guidedController.landTitle
    message:    _guidedController.landMessage
    iconSource: "/res/land.svg"
    visible:    _guidedController.takeoffLandSlotShowsLand
    enabled:    _guidedController.showLand
    actionID:   _guidedController.actionLand
}
