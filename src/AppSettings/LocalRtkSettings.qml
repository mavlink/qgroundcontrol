import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading: qsTr("Local RTK Receiver")
    visible: root._settings.userVisible

    readonly property var _settings: QGroundControl.settingsManager.rtkSettings
    readonly property var _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    readonly property var _manager: QGroundControl.gpsManager
    readonly property var _connection: root._manager.rtkConnection
    readonly property var _facts: QGroundControl.gpsRtk
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property var _serialPorts: root._serialPortManager ? root._serialPortManager.serialPorts : []
    readonly property bool _serial: root._settings.connectionType.rawValue === RTKSettings.Serial
    readonly property bool _udp: root._settings.connectionType.rawValue === RTKSettings.Udp
    readonly property bool _active: root._connection.active
    property bool _invalidConnection: false

    GpsConnectionType {
        objectName: "rtkConnectionType"
        Layout.fillWidth: true
        fact: root._settings.connectionType
        excludedValues: root._serialPortManager ? [] : [RTKSettings.Serial]
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        wrapMode: Text.WordWrap
        text: root._serial
              ? qsTr("Choose a serial device or discover a supported USB RTK receiver automatically. The receiver baud rate is detected automatically.")
              : root._udp
                ? qsTr("Connect to an RTK receiver or bidirectional serial bridge over UDP. The receiver must accept configuration commands and return data from the configured host and port. Serial bridges must use 115200 baud.")
                : qsTr("Connect to an RTK receiver over TCP using the host, port, and receiver type below.")
    }

    LabelledComboBox {
        objectName: "rtkSerialDevice"
        Layout.fillWidth: true
        visible: root._serial
        enabled: !root._active
        label: qsTr("Device")
        model: [qsTr("Automatic discovery")].concat(root._serialPorts)
        currentIndex: root._settings.serialDevice.valueString === "" ? 0
                      : (root._serialPorts.indexOf(root._settings.serialDevice.valueString) < 0 ? -1
                         : root._serialPorts.indexOf(root._settings.serialDevice.valueString) + 1)
        onActivated: index => {
            if (index >= 0 && index <= root._serialPorts.length) {
                root._settings.serialDevice.rawValue = index === 0 ? "" : root._serialPorts[index - 1]
            }
        }
    }

    LabelledFactTextField {
        objectName: "networkRtkHost"
        Layout.fillWidth: true
        textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
        visible: !root._serial
        label: qsTr("Host")
        fact: root._settings.networkBaseHost
        enabled: !root._active
    }

    LabelledFactTextField {
        objectName: "networkRtkPort"
        Layout.fillWidth: true
        visible: !root._serial
        label: qsTr("Port")
        fact: root._settings.networkBasePort
        enabled: !root._active
    }

    LabelledFactTextField {
        objectName: "networkRtkLocalPort"
        Layout.fillWidth: true
        visible: root._udp
        label: qsTr("Local port (0 = automatic)")
        fact: root._settings.udpLocalPort
        enabled: !root._active
    }

    LabelledFactComboBox {
        objectName: "networkRtkType"
        Layout.fillWidth: true
        visible: !root._serial || root._settings.serialDevice.valueString !== ""
        label: qsTr("Receiver")
        fact: root._settings.networkReceiverType
        enabled: !root._active
    }

    FactCheckBox {
        objectName: "rtkUseReceiverPosition"
        Layout.fillWidth: true
        text: qsTr("Use receiver for ground-station position")
        fact: root._settings.useReceiverPosition
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        visible: root._settings.useReceiverPosition.rawValue
        wrapMode: Text.WordWrap
        text: qsTr("Place the receiver with the ground station. This uses live position fixes available in the receiver's current mode. When disconnected, the configured NMEA or device position source is used.")
    }

    GpsConnectionControls {
        autoConnectFact: root._serial ? root._autoConnectSettings.autoConnectRTKGPS
                                     : root._autoConnectSettings.autoConnectNetworkRTKGPS
        autoConnectObjectName: root._serial ? "serialRtkAutoConnect" : "networkRtkAutoConnect"
        connectButtonObjectName: "networkRtkConnectButton"
        statusObjectName: "networkRtkStatus"
        active: root._active
        available: root._serial ? root._serialPortManager !== null
                               : root._settings.networkBaseHost.valueString.trim().length > 0
        statusText: {
            if (root._invalidConnection) return qsTr("Unable to connect. Check the connection settings.")
            if (!root._active) return root._connection.autoConnectPaused ? qsTr("Automatic connection paused") : qsTr("Disconnected")
            if (root._facts.connected.value) return qsTr("Connected")
            if (root._facts.lastError.value) return qsTr("%1 — reconnecting").arg(root._facts.lastError.enumStringValue)
            return root._serial ? qsTr("Waiting for receiver") : qsTr("Connecting")
        }
        onConnectRequested: root._invalidConnection = !root._manager.connectRtk()
        onDisconnectRequested: {
            root._invalidConnection = false
            root._manager.disconnectRtk()
        }
    }
}
