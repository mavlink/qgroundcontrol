import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("Connection")
    visible:            _ntrip.userVisible

    QGCPalette { id: qgcPal }

    property var  _ntrip:           QGroundControl.settingsManager.ntripSettings
    property Fact _enabled:         _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr:        QGroundControl.ntripManager
    property bool _isConnected:     _ntripMgr.connectionStatus === NTRIPManager.Connected
    property bool _isConnecting:    _ntripMgr.connectionStatus === NTRIPManager.Connecting
    property bool _isReconnecting:  _ntripMgr.connectionStatus === NTRIPManager.Reconnecting
    property bool _isError:         _ntripMgr.connectionStatus === NTRIPManager.Error
    property bool _isActive:        _enabled.rawValue
    property bool _hasHost:         _ntrip.ntripServerHostAddress.rawValue !== ""

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth

        Rectangle {
            width:          ScreenTools.defaultFontPixelHeight * 0.75
            height:         width
            radius:         width / 2
            color: {
                switch (_ntripMgr.connectionStatus) {
                case NTRIPManager.Connected:    return qgcPal.colorGreen
                case NTRIPManager.Connecting:
                case NTRIPManager.Reconnecting: return qgcPal.colorOrange
                case NTRIPManager.Error:        return qgcPal.colorRed
                default:                        return qgcPal.colorGrey
                }
            }
            Layout.alignment: Qt.AlignVCenter
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               _ntripMgr.statusMessage || qsTr("Disconnected")
            wrapMode:           Text.WordWrap
        }

        QGCButton {
            text: {
                if (_isConnecting)      return qsTr("Connecting…")
                if (_isReconnecting)    return qsTr("Reconnecting…")
                if (_isConnected)       return qsTr("Disconnect")
                return qsTr("Connect")
            }
            enabled:    !_isConnecting && (_isActive || _hasHost)
            onClicked:  _enabled.rawValue = !_enabled.rawValue
        }
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _isConnected && _ntripMgr.messagesReceived > 0
        text: {
            var parts = [qsTr("%1 messages").arg(_ntripMgr.messagesReceived),
                         formatBytes(_ntripMgr.bytesReceived)]
            if (_ntripMgr.dataRateBytesPerSec > 0)
                parts.push(formatRate(_ntripMgr.dataRateBytesPerSec))
            if (_ntripMgr.ggaSource)
                parts.push(qsTr("GGA: %1").arg(_ntripMgr.ggaSource))
            return parts.join(" · ")
        }
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.colorGrey

        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + " B"
            if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
            return (bytes / 1048576).toFixed(1) + " MB"
        }

        function formatRate(bytesPerSec) {
            if (bytesPerSec < 1024) return bytesPerSec.toFixed(0) + " B/s"
            return (bytesPerSec / 1024).toFixed(1) + " KB/s"
        }
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _isError && _ntripMgr.statusMessage !== ""
        text:               _ntripMgr.statusMessage
        wrapMode:           Text.WordWrap
        color:              qgcPal.colorRed
        font.pointSize:     ScreenTools.smallFontPointSize
    }
}
