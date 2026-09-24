import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

NmeaGpsSettings {
    heading:            qsTr("NMEA External GPS")
    Layout.fillWidth:   true
    visible:            !ScreenTools.isMobile && nmeaSettingsVisible
                        && QGroundControl.settingsManager.remoteIDSettings.locationType.value !== RemoteIDSettings.LocationType.TAKEOFF
}
