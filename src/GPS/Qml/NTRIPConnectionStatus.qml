pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// NTRIP connection status and control: connection-state row with the
/// connect/retry action, the disconnect-on-error action, and live stream details.
SettingsGroupLayout {
    id: root

    property NTRIPManager ntripManager: QGroundControl.gpsManager.ntrip
    property Fact enabledFact: root._ntripSettings.ntripServerConnectEnabled
    /// Supplies the bytes queued to vehicle links.
    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    readonly property NTRIPSettings _ntripSettings: QGroundControl.settingsManager.ntripSettings
    readonly property bool   _isActive:  root.enabledFact.rawValue
    readonly property bool   _hasHost:   root._ntripSettings.ntripServerHostAddress.rawValue !== ""
    readonly property int    _status:    root.ntripManager.connectionStatus
    readonly property NTRIPConnectionStats _stats: root.ntripManager.connectionStats
    readonly property bool   _connected: root._status === NTRIPManager.Connected
    readonly property string _ggaSource: root.ntripManager.ggaSource
    readonly property string _securityWarning: root.ntripManager.securityWarning
    readonly property string _mountpoint: root._ntripSettings.ntripMountpoint.valueString

    heading: qsTr("NTRIP Connection")
    visible: root._ntripSettings.userVisible
    showDividers: false

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
        statusText: root.ntripManager.statusMessage || root.ntripManager.connectionStatusText
        buttonText: {
            switch (root._status) {
            case NTRIPManager.Connecting:   return qsTr("Connecting…")
            case NTRIPManager.Reconnecting: return qsTr("Cancel reconnect")
            case NTRIPManager.Connected:    return qsTr("Disconnect")
            case NTRIPManager.Error:        return qsTr("Retry")
            default:                        return qsTr("Connect")
            }
        }
        buttonEnabled: root._status !== NTRIPManager.Connecting && (root._isActive || root._hasHost)
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

    GPSNoteLabel {
        text:           qsTr("Connected but no data received recently")
        warning:        true
        visible:        root._stats.dataStale && root._connected
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
        labelText: qsTr("%1 s ago").arg(root._stats.correctionAgeSec.toFixed(1))
        visible:   root._connected && root._stats.correctionAgeSec >= 0
    }

    LabelledLabel {
        label:     qsTr("Messages")
        labelText: root._stats.messagesReceived
        visible:   root._connected && root._stats.messagesReceived > 0
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root._connected && root._stats.messageCountsById.length > 0

        QGCLabel {
            text: qsTr("Message Types")
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorGrey
        }

        RTCMMessageChips {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: 0
            messageCounts:    root._stats.messageCountsById
        }
    }

    LabelledLabel {
        label:     qsTr("Data Received")
        //: %1 is the total data size, %2 the data size received per second
        labelText: qsTr("%1 (%2/s)").arg(QGroundControl.bigSizeToString(root._stats.bytesReceived))
                                    .arg(QGroundControl.bigSizeToString(root._stats.dataRateBytesPerSec))
        visible:   root._connected && root._stats.bytesReceived > 0
    }

    GPSNoteLabel {
        text:           qsTr("Warning: Data usage: %1 — consider connection costs").arg(QGroundControl.bigSizeToString(root._stats.bytesReceived))
        warning:        true
        visible:        root._connected && root._stats.dataUsageHigh
    }

    LabelledLabel {
        label:     qsTr("Queued to vehicle links (any source)")
        labelText: QGroundControl.bigSizeToString(root.corrections.vehicleBytesSubmitted)
        visible:   root._connected && root.corrections.vehicleBytesSubmitted > 0
    }

    LabelledLabel {
        label:           qsTr("GGA Source")
        labelText:       root._ggaSource
        labelTextFormat: Text.PlainText
        visible:         root._connected && root._ggaSource !== ""
    }

    GPSNoteLabel {
        objectName:     "ntripNoGgaPosition"
        text:           qsTr("No position is available to send to the caster. Network (VRS) mountpoints send corrections only after receiving a position; check the GGA position source.")
        warning:        true
        visible:        root._connected && root._ggaSource === ""
                        && (root._stats.dataStale || root._stats.messagesReceived === 0)
    }

    GPSNoteLabel {
        text:           root._securityWarning
        warning:        true
        visible:        root._connected && root._securityWarning !== ""
    }
}
