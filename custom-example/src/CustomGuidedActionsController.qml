// Custom builds can override this file to add custom guided actions.

import QtQml

import QGroundControl

QtObject {
    id: _root
    readonly property int actionCustomButton: _guidedController.customActionStart + 0
    readonly property string customButtonTitle: qsTr("Custom")
    readonly property string customButtonMessage: qsTr("Example of a custom action.")

    function customConfirmAction(actionCode, actionData, mapIndicator, confirmDialog) {
        switch (actionCode) {
        case actionCustomButton:
            confirmDialog.hideTrigger = true
            confirmDialog.title = customButtonTitle
            confirmDialog.message = customButtonMessage
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
        default:
            return false // false = action not handled here
        }

        return true // true = action handled here
    }
}
