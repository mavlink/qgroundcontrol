import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    name:                   qsTr("Sound output")
    manualText:             qsTr("QGC audio output enabled. System audio output enabled, too?")
    telemetryTextFailure:   qsTr("QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!")
    telemetryFailure:       QGroundControl.settingsManager.appSettings.audioMuted.rawValue
}
