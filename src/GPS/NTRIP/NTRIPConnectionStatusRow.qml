import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS.NTRIP

/// Status row specialized for an NTRIPManager. Bakes in the NTRIP
/// connection-state → (status color, button label) mapping so callers
/// only need to supply the manager and an action handler.
///
/// Example:
///   NTRIPConnectionStatusRow {
///       ntripManager: QGroundControl.ntripManager
///       canConnect:   _isActive || _hasHost
///       onButtonClicked: _enabled.rawValue = !_enabled.rawValue
///   }
ConnectionStatusRow {
    id: root

    required property var ntripManager

    /// Caller-supplied gate for the connect button (e.g. "host field non-empty").
    /// The Connecting state is still forced-disabled regardless of this flag.
    property bool canConnect: true

    QGCPalette { id: _pal }

    statusColor: {
        if (!root.ntripManager) return _pal.colorGrey
        switch (root.ntripManager.connectionStatus) {
        case NTRIPManager.Connected:    return _pal.colorGreen
        case NTRIPManager.Connecting:
        case NTRIPManager.Reconnecting: return _pal.colorOrange
        case NTRIPManager.Error:        return _pal.colorRed
        default:                        return _pal.colorGrey
        }
    }
    statusText: {
        if (!root.ntripManager) return qsTr("Unavailable")
        return root.ntripManager.statusMessage || qsTr("Disconnected")
    }
    buttonText: {
        if (!root.ntripManager) return ""
        switch (root.ntripManager.connectionStatus) {
        case NTRIPManager.Connecting:   return qsTr("Connecting…")
        case NTRIPManager.Reconnecting: return qsTr("Reconnecting…")
        case NTRIPManager.Connected:    return qsTr("Disconnect")
        default:                        return qsTr("Connect")
        }
    }
    buttonEnabled: root.ntripManager
                   && root.ntripManager.connectionStatus !== NTRIPManager.Connecting
                   && root.canConnect
}
