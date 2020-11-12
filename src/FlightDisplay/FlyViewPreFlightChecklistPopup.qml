/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl           1.0
import QGroundControl.Vehicle   1.0

/// Popup container for preflight checklists
Popup {
    id:             _root
    height:         checkList.height
    width:          checkList.width
    modal:          true
    focus:          true
    closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0)
        clip:           true
    }

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _useChecklist:      QGroundControl.settingsManager.appSettings.useChecklist.rawValue && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
    property bool   _enforceChecklist:  _useChecklist && QGroundControl.settingsManager.appSettings.enforceChecklist.rawValue
    property bool   _checklistComplete: _activeVehicle && (_activeVehicle.checkListState === Vehicle.CheckListPassed)

    on_ActiveVehicleChanged: _showPreFlightChecklistIfNeeded()

    Connections {
        target:                             mainWindow
        onShowPreFlightChecklistIfNeeded:   _root._showPreFlightChecklistIfNeeded()
    }

    function _showPreFlightChecklistIfNeeded() {
        if (_activeVehicle && !_checklistComplete && _enforceChecklist) {
            popupTimer.restart()
        }
    }

    Timer {
        id:             popupTimer
        interval:       1000
        repeat:         false
        onTriggered: {
            // FIXME: What was the visible check in here for
            if (!_checklistComplete) {
                console.log("open", _root.width, _root.height)
                _root.open()
            } else {
                _root.close()
            }
        }
    }

    Loader {
        id:                 checkList
        anchors.centerIn:   parent
        source:             QGroundControl.corePlugin.options.preFlightChecklistUrl
    }

    property alias checkListItem: checkList.item

    Connections {
        target: checkList.item
        onAllChecksPassedChanged: {
            if (target.allChecksPassed) {
                popupTimer.restart()
            }
        }
    }
}
