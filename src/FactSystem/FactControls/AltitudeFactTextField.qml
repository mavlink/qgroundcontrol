import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

FactTextField {
    unitsLabel:                 fact ? fact.units : ""
    extraUnitsLabel:            fact ? _altitudeModeExtraUnits : ""
    showUnits:                  true
    showHelp:                   true

    property int altitudeMode: QGroundControl.AltitudeModeNone

    property string _altitudeModeExtraUnits

    onAltitudeModeChanged: updateAltitudeModeExtraUnits()

    function updateAltitudeModeExtraUnits() {
        _altitudeModeExtraUnits = QGroundControl.altitudeModeExtraUnits(altitudeMode);
    }
}
