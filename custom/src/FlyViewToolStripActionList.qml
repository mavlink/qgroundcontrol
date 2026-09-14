import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    model: [
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        GuidedActionPause { },
        FlyViewAdditionalActionsButton { },
        GuidedToolStripAction {
            text:       _guidedController._customController.customButtonTitle
            iconSource: "/res/gear-white.svg"
            visible:    true
            enabled:    true
            actionID:   _guidedController._customController.actionCustomButton
        },
        GuidedToolStripAction {
            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            text:       _guidedController._customController.assumeRoverControlTitle
            iconSource: "/res/gear-white.svg"
            visible:    _activeVehicle && _activeVehicle.rover && _activeVehicle.flightMode !== "Manual"
            enabled:    visible
            actionID:   _guidedController._customController.actionAssumeRoverControl
        },
        GuidedToolStripAction {
            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            text:       _guidedController._customController.returnToAutoTitle
            iconSource: "/res/gear-white.svg"
            visible:    _activeVehicle && _activeVehicle.rover && _activeVehicle.flightMode === "Manual"
            enabled:    visible
            actionID:   _guidedController._customController.actionReturnToAuto
        }
    ]
}
