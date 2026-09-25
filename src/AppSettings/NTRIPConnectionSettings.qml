import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

SettingsGroupLayout {
    id: root

    Layout.fillWidth:   true
    heading:            qsTr("NTRIP Connection")
    visible:            _ntrip.userVisible

    // NTRIPSettings is not registered for QML, and SettingsGroup is not either.
    property var  _ntrip:    QGroundControl.settingsManager.ntripSettings
    property Fact _enabled:  _ntrip.ntripServerConnectEnabled
    property NTRIPManager _ntripMgr: QGroundControl.gpsManager.ntrip
    readonly property GPSCorrectionManager _corrections: QGroundControl.gpsManager.corrections

    NTRIPConnectionStatus {
        Layout.fillWidth: true
        ntripManager:     root._ntripMgr
        enabledFact:      root._enabled
        canConnect:       root._ntrip.ntripServerHostAddress.rawValue !== ""
        corrections:      root._corrections
    }
}
