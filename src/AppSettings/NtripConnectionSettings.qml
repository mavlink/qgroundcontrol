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

    property var  _ntrip:    QGroundControl.settingsManager.ntripSettings
    property Fact _enabled:  _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr: QGroundControl.gpsManager.ntrip
    readonly property var _corrections: QGroundControl.gpsManager.corrections

    NTRIPConnectionStatus {
        Layout.fillWidth: true
        ntripManager:     root._ntripMgr
        enabledFact:      root._enabled
        canConnect:       root._ntrip.ntripServerHostAddress.rawValue !== ""
        rtcmMavlink:      root._corrections ? root._corrections.rtcmMavlink : null
    }
}
