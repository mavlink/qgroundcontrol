// Custom builds can override this file to add custom guided actions.

import QtQml

QtObject {
    function customConfirmAction(actionCode, actionData, mapIndicator, confirmDialog) {
        return false // false = action not handled here
    }

    function customExecuteAction(actionCode, actionData, sliderOutputValue, optionCheckedode) {
        return false // false = action not handled here
    }
}
