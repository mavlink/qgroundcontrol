import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// The GNSS receiver's connection: a row with its state, what the receiver is doing and the action that connects it or
/// stops it, and below the row the receiver's connection or input problem.
ColumnLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// What the row says without a receiver.
    property string disconnectedText: qsTr("No GNSS receiver connected.")

    readonly property int _state: root.receiver.state
    readonly property string _problem: root.receiver.errorMessage

    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCPalette { id: qgcPal }

    ConnectionStatusRow {
        objectName: "rtkReceiverStatus"
        Layout.fillWidth: true
        buttonObjectName: "rtkConnectButton"
        statusColor: {
            switch (root._state) {
            case GPSReceiver.Connected:    return root._problem !== "" ? qgcPal.colorOrange : qgcPal.colorGreen
            case GPSReceiver.Disconnected: return root._problem !== "" ? qgcPal.colorRed : qgcPal.colorGrey
            default:                       return qgcPal.colorOrange
            }
        }
        statusText: root.receiver.statusText || root.disconnectedText
        buttonText: {
            switch (root._state) {
            case GPSReceiver.Disconnected: return qsTr("Connect")
            case GPSReceiver.Reconnecting: return qsTr("Cancel reconnect")
            default:                       return qsTr("Disconnect")
            }
        }
        // Connect needs settings that can connect; a connection being opened can be stopped, as a reconnect can.
        buttonEnabled: root._state !== GPSReceiver.Disconnected || root.receiver.canConnect
        onClicked: {
            if (root._state === GPSReceiver.Disconnected) {
                root.receiver.connectReceiver()
            } else {
                root.receiver.disconnectReceiver()
            }
        }
    }

    // Below the row, where it has the whole width: beside the button a long message would wrap into a narrow column.
    QGCNoteLabel {
        objectName: "rtkErrorMessage"
        visible: text !== ""
        text: root._problem
    }
}
