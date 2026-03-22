import QtQuick

import QGroundControl
import QGroundControl.Controls

PreFlightCheckButton {
    name:                   qsTr("Sound output")
    manualText:             qsTr("QGC audio output enabled. System audio output enabled, too?")
    telemetryTextFailure:   _audioMuted ? qsTr("QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!")
                                        : qsTr("QGC audio volume is set to 0%. Please increase volume under application settings->general to hear audio warnings!")
    telemetryFailure:       _audioMuted || _audioVolume <= 0.0

    property bool _audioMuted: QGroundControl.settingsManager.appSettings.audioMuted.rawValue
    property real _audioVolume: QGroundControl.settingsManager.appSettings.audioVolume.rawValue
}
