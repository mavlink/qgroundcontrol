pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// NTRIP connection status and control: connection-state row with the
/// connect/retry action, the disconnect-on-error action, and live stream details.
QGCGroupBox {
    id: root

    property NTRIPManager ntripManager: QGroundControl.gpsManager.ntrip
    /// Supplies the bytes queued to vehicle links.
    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    readonly property NTRIPSettings _ntripSettings: QGroundControl.settingsManager.ntripSettings
    readonly property int    _status:    root.ntripManager.connectionStatus
    readonly property NTRIPConnectionStats _stats: root.ntripManager.connectionStats
    readonly property bool   _connected: root._status === NTRIPManager.Connected
    /// The row's action starts a connection in these states and ends it in the others.
    readonly property bool   _startsConnection: root._status === NTRIPManager.Disconnected
                                                || root._status === NTRIPManager.Error
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
        statusText: root.ntripManager.statusMessage
        buttonText: {
            switch (root._status) {
            case NTRIPManager.Disconnected: return qsTr("Connect")
            case NTRIPManager.Error:        return qsTr("Retry")
            case NTRIPManager.Reconnecting: return qsTr("Cancel reconnect")
            default:                        return qsTr("Disconnect")
            }
        }
        // Connect and Retry need a caster to connect to; a connection being opened can be stopped, as a reconnect can.
        buttonEnabled: !root._startsConnection || root.ntripManager.canConnect
        // Explicit targets keep repeated clicks idempotent while the manager debounces the setting change.
        onClicked: {
            if (root._startsConnection) {
                root.ntripManager.connectCaster()
            } else {
                root.ntripManager.disconnectCaster()
            }
        }
    }

    QGCButton {
        objectName: "ntripDisconnectButton"
        text:       qsTr("Disconnect")
        visible:    root._status === NTRIPManager.Error
        onClicked:  root.ntripManager.disconnectCaster()
    }

    QGCNoteLabel {
        text:           qsTr("Connected but no data received recently")
        warning:        true
        visible:        root._stats.dataStale && root._connected
    }

    // The mountpoint, the data received and the data queued to vehicles can be wider than the status column of the
    // GPS indicator's drop-down has room for, so the values of these rows wrap.
    GPSWrappingRow {
        label:           qsTr("Mountpoint")
        labelText:       root._mountpoint
        visible:         root._mountpoint !== "" && root._connected
    }

    LabelledLabel {
        objectName: "ntripCorrectionAge"
        label:     qsTr("Last correction")
        //: %1 is the time in seconds since the last correction was received
        labelText: qsTr("%1 s ago").arg(root._stats.correctionAgeSec.toFixed(1))
        labelTextFormat: Text.PlainText
        visible:   root._connected && root._stats.correctionAgeSec >= 0
    }

    LabelledLabel {
        label:     qsTr("Messages")
        labelText: root._stats.messagesReceived
        labelTextFormat: Text.PlainText
        visible:   root._connected && root._stats.messagesReceived > 0
    }

    GPSWrappingRow {
        objectName: "ntripDataReceived"
        label:     qsTr("Data Received")
        //: %1 is the total data size, %2 the data size received per second
        labelText: qsTr("%1 (%2/s)").arg(QGroundControl.bigSizeToString(root._stats.bytesReceived))
                                    .arg(QGroundControl.bigSizeToString(root._stats.dataRateBytesPerSec))
        labelWrapMode: Text.Wrap
        visible:   root._connected && root._stats.bytesReceived > 0
    }

    QGCNoteLabel {
        text:           qsTr("Warning: Data usage: %1 — consider connection costs").arg(QGroundControl.bigSizeToString(root._stats.bytesReceived))
        warning:        true
        visible:        root._connected && root._stats.dataUsageHigh
    }

    GPSWrappingRow {
        objectName: "ntripQueuedToVehicles"
        label:     qsTr("Queued to vehicles")
        //: %1 is the size of the corrections queued to vehicles, from whichever source they came. The space in
        //: "any source" is a no-break space.
        labelText: qsTr("%1 (any\u00A0source)")
                       .arg(QGroundControl.bigSizeToString(root.corrections.vehicleBytesSubmitted))
        labelWrapMode: Text.Wrap
        visible:   root._connected && root.corrections.vehicleBytesSubmitted > 0
    }

    LabelledLabel {
        label:           qsTr("GGA Source")
        labelText:       root._ggaSource
        labelTextFormat: Text.PlainText
        visible:         root._connected && root._ggaSource !== ""
    }

    QGCNoteLabel {
        objectName:     "ntripNoGgaPosition"
        text:           qsTr("No position is available to send to the caster. Network (VRS) mountpoints send corrections only after receiving a position; check the GGA position source.")
        warning:        true
        visible:        root._connected && root._ggaSource === ""
                        && (root._stats.dataStale || root._stats.messagesReceived === 0)
    }

    QGCNoteLabel {
        text:           root._securityWarning
        warning:        true
        visible:        root._connected && root._securityWarning !== ""
    }
}
