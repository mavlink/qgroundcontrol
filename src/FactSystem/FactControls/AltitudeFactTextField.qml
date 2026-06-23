import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

FactTextField {
    unitsLabel:                 fact ? fact.units : ""
    extraUnitsLabel:            fact ? qsTr("%1").arg(_altitudeFrameExtraUnits) : ""
    showUnits:                  true
    showHelp:                   true

    property int altitudeFrame: QGroundControl.AltitudeFrameNone

    property string _altitudeFrameExtraUnits

    onAltitudeFrameChanged: updateAltitudeFrameExtraUnits()

    function updateAltitudeFrameExtraUnits() {
        _altitudeFrameExtraUnits = QGroundControl.altitudeFrameExtraUnits(altitudeFrame);
    }
}
