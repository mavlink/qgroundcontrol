import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCButton {
    objectName: "remoteIdConfigureGpsSourceButton"
    text: qsTr("Configure GPS source")
    visible: !ScreenTools.isMobile
             && QGroundControl.settingsManager.autoConnectSettings.nmeaSource.userVisible
             && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.userVisible
             && QGroundControl.settingsManager.remoteIDSettings.locationType.rawValue === RemoteIDSettings.LocationType.LIVE

    // Navigation keys are the untranslated names in the settings definitions.
    onClicked: mainWindow.showSettingsTool("GPS", "NMEA GPS")
}
