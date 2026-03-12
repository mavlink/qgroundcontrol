import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    property var rtcmMavlink: null

    property var    _ntripMgr:      QGroundControl.ntripManager
    property int    _lastMsgCount:  0
    property bool   _dataStale:     false
    property string _valueNA:       qsTr("-.--")

    heading:      qsTr("NTRIP Corrections")
    showDividers: true
    visible:      _ntripMgr.connectionStatus !== NTRIPManager.Disconnected

    function _formatDataSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / 1048576).toFixed(1) + " MB"
    }

    function _formatDataRate(bytesPerSec) {
        if (bytesPerSec < 1024) return bytesPerSec.toFixed(0) + " B/s"
        return (bytesPerSec / 1024).toFixed(1) + " KB/s"
    }

    QGCPalette { id: qgcPal }

    Timer {
        interval:  5000
        running:   _ntripMgr.connectionStatus === NTRIPManager.Connected
        repeat:    true
        onTriggered: {
            if (_ntripMgr.connectionStats.messagesReceived === _lastMsgCount && _ntripMgr.connectionStats.messagesReceived > 0) {
                _dataStale = true
            } else {
                _dataStale = false
            }
            _lastMsgCount = _ntripMgr.connectionStats.messagesReceived
        }
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        Rectangle {
            width:  ScreenTools.defaultFontPixelHeight * 0.6
            height: width
            radius: width / 2
            color: {
                if (_dataStale) return qgcPal.colorOrange
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
            text: {
                if (_dataStale && _ntripMgr.connectionStatus === NTRIPManager.Connected)
                    return qsTr("No Data")
                switch (_ntripMgr.connectionStatus) {
                case NTRIPManager.Connected:    return qsTr("Connected")
                case NTRIPManager.Connecting:   return qsTr("Connecting")
                case NTRIPManager.Reconnecting: return qsTr("Reconnecting")
                case NTRIPManager.Error:        return qsTr("Error")
                default:                        return qsTr("Disconnected")
                }
            }
        }
    }

    LabelledLabel {
        label:     qsTr("Mountpoint")
        labelText: QGroundControl.settingsManager.ntripSettings.ntripMountpoint.valueString
        visible:   QGroundControl.settingsManager.ntripSettings.ntripMountpoint.valueString !== ""
                   && _ntripMgr.connectionStatus === NTRIPManager.Connected
    }

    LabelledLabel {
        label:     qsTr("Messages")
        labelText: _ntripMgr.connectionStats.messagesReceived
        visible:   _ntripMgr.connectionStatus === NTRIPManager.Connected
                   && _ntripMgr.connectionStats.messagesReceived > 0
    }

    LabelledLabel {
        label:     qsTr("Data Received")
        labelText: _formatDataSize(_ntripMgr.connectionStats.bytesReceived) + " ("
                   + _formatDataRate(_ntripMgr.connectionStats.dataRateBytesPerSec) + ")"
        visible:   _ntripMgr.connectionStatus === NTRIPManager.Connected
                   && _ntripMgr.connectionStats.bytesReceived > 0
    }

    LabelledLabel {
        label:     qsTr("To Vehicle")
        labelText: rtcmMavlink ? (_formatDataSize(rtcmMavlink.totalBytesSent) + " ("
                   + rtcmMavlink.bandwidthKBps.toFixed(1) + " KB/s)") : _valueNA
        visible:   _ntripMgr.connectionStatus === NTRIPManager.Connected
                   && rtcmMavlink && rtcmMavlink.totalBytesSent > 0
    }

    LabelledLabel {
        label:     qsTr("GGA Source")
        labelText: _ntripMgr.ggaSource
        visible:   _ntripMgr.connectionStatus === NTRIPManager.Connected
                   && _ntripMgr.ggaSource
    }

    QGCLabel {
        text:           _ntripMgr.statusMessage
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorRed
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        _ntripMgr.connectionStatus === NTRIPManager.Error
                        && _ntripMgr.statusMessage !== ""
        Layout.fillWidth: true
    }
}
