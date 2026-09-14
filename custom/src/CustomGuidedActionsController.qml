// Custom builds can override this file to add custom guided actions.

import QtQml

import QGroundControl

QtObject {
    id: _root
    readonly property int actionCustomButton: _guidedController.customActionStart + 0
    readonly property string customButtonTitle: qsTr("Custom")
    readonly property string customButtonMessage: qsTr("Example of a custom action.")

    // Rover manual takeover (Decision 1, pure Design B - no fail-open bypass; the rover's own
    // link-loss failsafe and BendyRuler obstacle avoidance in Auto already cover the emergency
    // cases a bypass would exist for). Deliberately does NOT check multi-GCS control permission
    // (gcsControlStatusFlags_TakeoverAllowed) - that answers "which ground station commands this
    // vehicle," a different problem from this single-GCS program's "should this operator drive
    // right now."
    readonly property int actionAssumeRoverControl: _guidedController.customActionStart + 1
    readonly property int actionReturnToAuto: _guidedController.customActionStart + 2
    readonly property string assumeRoverControlTitle: qsTr("Assume Rover Control")
    readonly property string assumeRoverControlMessage: qsTr("Switch the rover to manual driving control?")
    readonly property string returnToAutoTitle: qsTr("Return to Auto")
    readonly property string returnToAutoMessage: qsTr("Return the rover to autonomous Auto mode?")

    // Looked up directly rather than relying on ambient _guidedController/_activeVehicle scoping,
    // since this object is a nested child of GuidedActionsController.qml, not a standalone file
    // that declares its own "property var _guidedController: globals.guidedControllerFlyView".
    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function customConfirmAction(actionCode, actionData, mapIndicator, confirmDialog) {
        switch (actionCode) {
        case actionCustomButton:
            confirmDialog.hideTrigger = true
            confirmDialog.title = customButtonTitle
            confirmDialog.message = customButtonMessage
            break
        case actionAssumeRoverControl:
            confirmDialog.hideTrigger = true
            confirmDialog.title = assumeRoverControlTitle
            confirmDialog.message = assumeRoverControlMessage
            break
        case actionReturnToAuto:
            confirmDialog.hideTrigger = true
            confirmDialog.title = returnToAutoTitle
            confirmDialog.message = returnToAutoMessage
            break
        default:
            return false // false = action not handled here
        }

        return true // true = action handled here
    }

    function customExecuteAction(actionCode, actionData, sliderOutputValue, optionCheckedode) {
        switch (actionCode) {
        case actionCustomButton:
            QGroundControl.showMessageDialog(mainWindow, "Custom Action", "Custom action executed.")
            break
        case actionAssumeRoverControl:
            if (!_activeVehicle) {
                break
            }
            if (!_activeVehicle.rover) {
                QGroundControl.showMessageDialog(mainWindow, assumeRoverControlTitle, qsTr("Active vehicle is not a rover."))
                break
            }
            if (_activeVehicle.vehicleLinkManager.communicationLost) {
                QGroundControl.showMessageDialog(mainWindow, assumeRoverControlTitle, qsTr("Cannot assume control: no telemetry from the rover."))
                break
            }
            if (_activeVehicle.flightModes.indexOf("Manual") === -1) {
                QGroundControl.showMessageDialog(mainWindow, assumeRoverControlTitle, qsTr("Rover does not report a Manual flight mode."))
                break
            }
            _activeVehicle.flightMode = "Manual"
            break
        case actionReturnToAuto:
            if (!_activeVehicle) {
                break
            }
            _activeVehicle.flightMode = "Auto"
            break
        default:
            return false // false = action not handled here
        }

        return true // true = action handled here
    }
}
