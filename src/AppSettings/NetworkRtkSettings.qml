import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading: qsTr("Network RTK Receiver")
    visible: root._settings.networkBaseHost.userVisible && root._settings.networkBasePort.userVisible
             && root._settings.networkReceiverType.userVisible

    readonly property var _settings: QGroundControl.settingsManager.rtkSettings
    readonly property var _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    readonly property var _manager: QGroundControl.gpsManager
    readonly property var _facts: QGroundControl.gpsRtk
    readonly property bool _active: root._manager.networkRtkActive
    property bool _invalidConnection: false

    QGCLabel {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Connect to a receiver command port over TCP. USB RTK autoconnect pauses until you disconnect. For a serial bridge, configure both the bridge and receiver to 115200 baud first.")
    }

    FactCheckBox {
        objectName: "networkRtkAutoConnect"
        Layout.fillWidth: true
        text: qsTr("Automatically connect to this receiver")
        fact: root._autoConnectSettings.autoConnectNetworkRTKGPS
        visible: fact.userVisible
        onClicked: {
            root.forceActiveFocus()
            root._invalidConnection = false
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        visible: root._autoConnectSettings.autoConnectNetworkRTKGPS.value
        wrapMode: Text.WordWrap
        text: qsTr("Connects at startup. Disconnect pauses automatic connection until you click Connect, turn this option off and on, or restart the application. Turning this option off disconnects the receiver.")
    }

    LabelledFactTextField {
        objectName: "networkRtkHost"
        Layout.fillWidth: true
        label: qsTr("Host")
        fact: root._settings.networkBaseHost
        enabled: !root._active
    }

    LabelledFactTextField {
        objectName: "networkRtkPort"
        Layout.fillWidth: true
        label: qsTr("Port")
        fact: root._settings.networkBasePort
        enabled: !root._active
    }

    LabelledFactComboBox {
        objectName: "networkRtkType"
        Layout.fillWidth: true
        label: qsTr("Receiver")
        fact: root._settings.networkReceiverType
        enabled: !root._active
    }

    QGCLabel {
        objectName: "networkRtkStatus"
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: {
            if (!root._active) {
                if (root._invalidConnection) return qsTr("Unable to connect. Check the host, port, receiver type, and whether connections are allowed.")
                if (root._autoConnectSettings.autoConnectNetworkRTKGPS.value) {
                    return root._manager.networkRtkAutoConnectPaused
                        ? qsTr("Automatic connection paused")
                        : qsTr("Waiting to connect automatically. Check the host, port, receiver type, and whether connections are allowed.")
                }
                return qsTr("Disconnected")
            }
            if (root._facts.connected.value) return qsTr("Connected")
            if (root._facts.lastError.value) return qsTr("%1 — retrying automatically").arg(root._facts.lastError.enumStringValue)
            return qsTr("Connecting…")
        }
    }

    QGCButton {
        objectName: "networkRtkConnectButton"
        text: root._active ? qsTr("Disconnect") : qsTr("Connect")
        enabled: root._active || root._settings.networkBaseHost.valueString.trim().length > 0
        onClicked: {
            root.forceActiveFocus()
            root._invalidConnection = false
            if (root._active) {
                root._manager.disconnectNetworkRtk()
            } else {
                root._invalidConnection = !root._manager.connectNetworkRtk()
            }
        }
    }
}
