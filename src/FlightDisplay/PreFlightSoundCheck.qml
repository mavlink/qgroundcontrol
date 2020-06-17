/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0

PreFlightCheckButton {
    name:                   qsTr("Sound output")
    manualText:             qsTr("QGC audio output enabled. System audio output enabled, too?")
    telemetryTextFailure:   qsTr("QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!")
    telemetryFailure:       QGroundControl.settingsManager.appSettings.audioMuted.rawValue
}
