/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0

PreFlightCheckButton {
    name:           qsTr("Sound output")
    pendingText:    qsTr("QGC audio output enabled. System audio output enabled, too?")
    failureText:    qsTr("Failure, QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!")

    property bool   _audioMuted:     QGroundControl.settingsManager.appSettings.audioMuted.rawValue
    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    on_AudioMutedChanged:       updateItem()
    on_ActiveVehicleChanged:    updateItem()

    Component.onCompleted: updateItem()

    function onActiveVehicleChanged() {
        buttonSoundOutput.updateItem();     // Just updated here for initialization once we connect to a vehicle
        updateVehicleDependentItems();
    }

    function updateItem() {
        if (!_activeVehicle) {
            state = stateNotChecked
        } else {
            if (_audioMuted) {
                state = stateMajorIssue
                _nrClicked = 0
            } else {
                state = _nrClicked > 0 ? statePassed : statePending
            }
        }
    }
}
