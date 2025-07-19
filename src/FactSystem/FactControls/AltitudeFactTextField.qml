/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
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
