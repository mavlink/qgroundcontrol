pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// NTRIP connection status and control: connection-state row with the
/// connect/retry action, the disconnect-on-error action, and live stream details.
ColumnLayout {
    id: root

    required property var  ntripManager
    required property Fact enabledFact

    /// Caller-supplied gate for the connect button (e.g. "host field non-empty").
    /// The Connecting state is still forced-disabled regardless of this flag.
    property bool canConnect: true
    property var  rtcmMavlink: null

    readonly property bool   _isActive:  root.enabledFact ? root.enabledFact.rawValue : false
    readonly property int    _status:    root.ntripManager ? root.ntripManager.connectionStatus : NTRIPManager.Disconnected
    readonly property var    _stats:     root.ntripManager ? root.ntripManager.connectionStats : null
    readonly property bool   _connected: root._status === NTRIPManager.Connected
    readonly property bool   _dataStale: root._stats ? root._stats.dataStale : false
    readonly property string _ggaSource: root.ntripManager && root.ntripManager.ggaSource ? root.ntripManager.ggaSource : ""
    readonly property string _securityWarning: root.ntripManager && root.ntripManager.securityWarning ? root.ntripManager.securityWarning : ""
    readonly property string _mountpoint: QGroundControl.settingsManager.ntripSettings.ntripMountpoint.valueString
    readonly property string _valueNA:   qsTr("-.--")

    readonly property real _dataWarningLimitBytes: 50 * 1024 * 1024  // 50 MB

    function _formatDataSize(bytes) {
        //: Data size in bytes
        if (bytes < 1024) return qsTr("%1 B").arg(bytes)
        //: Data size in kilobytes
        if (bytes < 1048576) return qsTr("%1 KB").arg((bytes / 1024).toFixed(1))
        //: Data size in megabytes
        return qsTr("%1 MB").arg((bytes / 1048576).toFixed(1))
    }

    spacing: ScreenTools.defaultFontPixelHeight / 2

    QGCPalette { id: qgcPal }

    ConnectionStatusRow {
        Layout.fillWidth: true
        buttonObjectName: "ntripConnectButton"
        statusColor: {
            switch (root._status) {
            case NTRIPManager.Connected:    return qgcPal.colorGreen
            case NTRIPManager.Connecting:
            case NTRIPManager.Reconnecting: return qgcPal.colorOrange
            case NTRIPManager.Error:        return qgcPal.colorRed
            default:                        return qgcPal.colorGrey
            }
        }
        statusText: {
            if (!root.ntripManager) return qsTr("Unavailable")
            return root.ntripManager.statusMessage || qsTr("Disconnected")
        }
        buttonText: {
            if (!root.ntripManager) return ""
            switch (root._status) {
            case NTRIPManager.Connecting:   return qsTr("Connecting…")
            case NTRIPManager.Reconnecting: return qsTr("Cancel reconnect")
            case NTRIPManager.Connected:    return qsTr("Disconnect")
            case NTRIPManager.Error:        return qsTr("Retry")
            default:                        return qsTr("Connect")
            }
        }
        buttonEnabled: !!root.ntripManager
                       && root._status !== NTRIPManager.Connecting
                       && (root._isActive || root.canConnect)
        // Explicit targets keep repeated clicks idempotent while the manager debounces the setting change.
        onClicked: {
            switch (root._status) {
            case NTRIPManager.Error:
                root.ntripManager.retryNTRIP()
                break
            case NTRIPManager.Disconnected:
                root.enabledFact.rawValue = true
                break
            default:
                root.enabledFact.rawValue = false
                break
            }
        }
    }

    QGCButton {
        objectName: "ntripDisconnectButton"
        text:       qsTr("Disconnect")
        visible:    root._isActive && root._status === NTRIPManager.Error
        onClicked:  root.enabledFact.rawValue = false
    }

    QGCLabel {
        text:             qsTr("Connected but no data received recently")
        color:            qgcPal.colorOrange
        wrapMode:         Text.WordWrap
        font.pointSize:   ScreenTools.smallFontPointSize
        visible:          root._dataStale && root._connected
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
    }

    LabelledLabel {
        label:           qsTr("Mountpoint")
        labelText:       root._mountpoint
        labelTextFormat: Text.PlainText
        visible:         root._mountpoint !== "" && root._connected
    }

    LabelledLabel {
        objectName: "ntripCorrectionAge"
        label:     qsTr("Last correction")
        //: %1 is the time in seconds since the last correction was received
        labelText: root._stats ? qsTr("%1 s ago").arg(root._stats.correctionAgeSec.toFixed(1)) : root._valueNA
        visible:   root._connected
                   && root._stats && root._stats.correctionAgeSec >= 0
    }

    LabelledLabel {
        label:     qsTr("Messages")
        labelText: root._stats ? root._stats.messagesReceived : ""
        visible:   root._connected
                   && root._stats && root._stats.messagesReceived > 0
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root._connected
                 && root._stats && root._stats.messageCountsById.length > 0

        QGCLabel {
            text: qsTr("Message Types")
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorGrey
        }

        RTCMMessageChips {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: 0
            messageCounts:    root._stats ? root._stats.messageCountsById : []
        }
    }

    LabelledLabel {
        label:     qsTr("Data Received")
        //: %1 is the total data size, %2 is the current data rate
        labelText: root._stats ? qsTr("%1 (%2)").arg(root._formatDataSize(root._stats.bytesReceived))
                                               .arg(GPSFormat.dataRate(root._stats.dataRateBytesPerSec))
                               : root._valueNA
        visible:   root._connected
                   && root._stats && root._stats.bytesReceived > 0
    }

    QGCLabel {
        text:           qsTr("Warning: Data usage: %1 — consider connection costs").arg(root._stats ? root._formatDataSize(root._stats.bytesReceived) : "")
        wrapMode:       Text.WordWrap
        color:          qgcPal.warningText
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._connected
                        && root._stats && root._stats.bytesReceived > root._dataWarningLimitBytes
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
    }

    LabelledLabel {
        label:     qsTr("Queued to vehicle links (any source)")
        labelText: root.rtcmMavlink ? root._formatDataSize(root.rtcmMavlink.totalBytesSubmitted) : root._valueNA
        visible:   root._connected
                   && root.rtcmMavlink && root.rtcmMavlink.totalBytesSubmitted > 0
    }

    LabelledLabel {
        label:           qsTr("GGA Source")
        labelText:       root._ggaSource
        labelTextFormat: Text.PlainText
        visible:         root._connected && root._ggaSource !== ""
    }

    QGCLabel {
        objectName:     "ntripNoGgaPosition"
        text:           qsTr("No position is available to send to the caster. Network (VRS) mountpoints send corrections only after receiving a position; check the GGA position source.")
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorOrange
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._connected && root.ntripManager && root._ggaSource === ""
                        && (!root._stats || root._stats.dataStale || root._stats.messagesReceived === 0)
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
    }

    QGCLabel {
        text:           root._securityWarning
        textFormat:     Text.PlainText
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorOrange
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._connected && root._securityWarning !== ""
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
    }
}
