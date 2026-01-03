import QGroundControl
import QGroundControl.FlyView

GuidedToolStripAction {
    text:       _guidedController.landTitle
    message:    _guidedController.landMessage
    iconSource: "/res/land.svg"
    visible:    _guidedController.showLand && !_guidedController.showTakeoff
    enabled:    _guidedController.showLand
    actionID:   _guidedController.actionLand
}
