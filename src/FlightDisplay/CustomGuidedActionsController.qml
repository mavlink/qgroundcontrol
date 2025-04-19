/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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