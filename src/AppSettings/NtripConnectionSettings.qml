import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.NTRIP

SettingsGroupLayout {
    id: root

    Layout.fillWidth:   true
    heading:            qsTr("Connection")
    visible:            _ntrip.userVisible

    property var  _ntrip:    QGroundControl.settingsManager.ntripSettings
    property Fact _enabled:  _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr: QGroundControl.ntripManager
    property bool _isActive: _enabled.rawValue
    property bool _hasHost:  _ntrip.ntripServerHostAddress.rawValue !== ""

    NTRIPConnectionStatusRow {
        Layout.fillWidth: true
        ntripManager:     root._ntripMgr
        canConnect:       root._isActive || root._hasHost
        onClicked:        root._enabled.rawValue = !root._isActive
    }

    NTRIPConnectionStatus {
        Layout.fillWidth: true
        rtcmMavlink:      root._ntripMgr ? root._ntripMgr.rtcmMavlink : null
    }
}
