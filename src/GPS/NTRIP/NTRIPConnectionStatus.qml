import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS
import QGroundControl.GPS.RTK

SettingsGroupLayout {
    id: root

    property var rtcmMavlink: null

    property var    _ntripMgr:      QGroundControl.ntripManager
    property bool   _dataStale:     _ntripMgr ? _ntripMgr.connectionStats.dataStale : false
    property string _valueNA:       qsTr("-.--")

    readonly property real _dataWarningLimitBytes: 50 * 1024 * 1024  // 50 MB

    heading:      qsTr("NTRIP Corrections")
    showDividers: true
    visible:      _ntripMgr ? _ntripMgr.connectionStatus !== NTRIPManager.Disconnected : false

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        FixStatusDot {
            statusColor: {
                if (_dataStale) return qgcPal.colorOrange
                switch (_ntripMgr.connectionStatus) {
                case NTRIPManager.Connected:    return qgcPal.colorGreen
                case NTRIPManager.Connecting:
                case NTRIPManager.Reconnecting: return qgcPal.colorOrange
                case NTRIPManager.Error:        return qgcPal.colorRed
                default:                        return qgcPal.colorGrey
                }
            }
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
        labelText: GPSFormatter.formatDataSize(_ntripMgr.connectionStats.bytesReceived) + " ("
                   + GPSFormatter.formatDataRate(_ntripMgr.connectionStats.dataRateBytesPerSec) + ")"
        visible:   _ntripMgr.connectionStatus === NTRIPManager.Connected
                   && _ntripMgr.connectionStats.bytesReceived > 0
    }

    QGCLabel {
        text:           qsTr("Warning: Data usage: %1 — consider connection costs").arg(GPSFormatter.formatDataSize(_ntripMgr.connectionStats.bytesReceived))
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorOrange
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        _ntripMgr.connectionStatus === NTRIPManager.Connected
                        && _ntripMgr.connectionStats.bytesReceived > _dataWarningLimitBytes
        Layout.fillWidth: true
    }

    LabelledLabel {
        label:     qsTr("To Vehicle")
        labelText: rtcmMavlink ? (GPSFormatter.formatDataSize(rtcmMavlink.totalBytesSent) + " ("
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
