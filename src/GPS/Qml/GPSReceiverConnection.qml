pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// What the receiver is used as, which receiver to connect and how to reach it, and its connection: Connect, with
/// what the receiver is doing and why a connection failed.
GPSReceiverSettingsGroup {
    id: root

    property list<gpsSerialPortEntry> serialPorts: root.receiver.serialPortEntries
    property list<int> serialBaudRates: root.receiver.serialBaudRates

    readonly property int _connection: settings.connectionType.rawValue
    readonly property bool _udp: receiver.effectiveConnectionType === RTKSettings.Udp
    readonly property bool _tcp: receiver.effectiveConnectionType === RTKSettings.Tcp
    readonly property bool _serial: receiver.effectiveConnectionType === RTKSettings.Serial
    readonly property bool _bluetooth: receiver.effectiveConnectionType === RTKSettings.Bluetooth
    readonly property bool _connectionSupported: receiver.connectionSupported

    heading: qsTr("GNSS Receiver")

    GPSReceiverUse {
        settings: root.settings
        editable: root._editable
    }

    GPSFactComboField {
        fact: root.settings.baseReceiverManufacturer
        shown: root.configuredBase
        comboBox.objectName: "rtkManufacturer"
        comboBox.enabled: root._editable

        QGCNoteLabel {
            objectName: "rtkAutomaticExplanation"
            visible: root.presentation.automatic
            text: root.settings.baseReceiverManufacturer.shortDescription
        }
    }

    GPSFactComboField {
        fact: root.settings.connectionType
        comboBox.objectName: "rtkConnectionType"
        comboBox.enabled: root._editable

        QGCNoteLabel {
            objectName: "rtkTcpOnly"
            visible: !root.receiver.serialSupported && root._connection === RTKSettings.Serial
            text: qsTr("Serial receivers are not supported on this platform, so the receiver connects over TCP.")
        }
    }

    GPSFactTextField {
        objectName: "rtkTcpHost"
        fact: root.settings.tcpHost
        enabled: root._editable
        shown: root._tcp
    }

    GPSFactTextField {
        objectName: "rtkTcpPort"
        fact: root.settings.tcpPort
        enabled: root._editable
        shown: root._tcp
        // The drivers configure a receiver at 115200 baud and cannot set a bridge's rate. A receiver that is only
        // listened to is taken at its own.
        note: root.configuredBase
              ? qsTr("Connect to a receiver's TCP port or a serial-to-TCP bridge. A bridge must already run the receiver link at 115200 baud; QGroundControl cannot change a bridge's rate.")
              : qsTr("Connect to a receiver's TCP port, or to a serial-to-TCP bridge that already runs the receiver link at the receiver's baud rate.")
    }

    GPSFactTextField {
        objectName: "rtkUdpPort"
        fact: root.settings.udpPort
        enabled: root._editable
        shown: root._udp

        QGCNoteLabel {
            objectName: "rtkUdpExplanation"
            warning: !root._connectionSupported
            text: root._connectionSupported ? root.settings.udpPort.shortDescription
                                            : qsTr("A configured base needs a serial, TCP or Bluetooth connection. UDP only receives data.")
        }
    }

    GPSReceiverSerialPort {
        Layout.minimumWidth: 0
        visible: root.receiver.serialSupported && root._serial
        deviceFact: root.settings.serialDevice
        baudFact: root.settings.serialBaudRate
        serialPorts: root.serialPorts
        serialBaudRates: root.serialBaudRates
        anyDeviceLabel: root.configuredBase && root.autoConnectFact.rawValue ? qsTr("Any RTK receiver on USB") : ""
        editable: root._editable
    }

    GPSReceiverBluetoothDevice {
        Layout.minimumWidth: 0
        visible: root._bluetooth
        addressFact: root.settings.bluetoothDeviceAddress
        nameFact: root.settings.bluetoothDeviceName
        // Asked for only once a Bluetooth connection is selected, as the receiver creates it then.
        discovery: root._bluetooth ? root.receiver.bluetoothDiscovery : null
        editable: root._editable
        configured: root.configuredBase
    }

    // Beside the device, which it can leave to USB discovery.
    GPSFactSwitchField {
        fact: root.autoConnectFact
        note: root.autoConnectFact.rawValue ? root.autoConnectFact.shortDescription : ""
        toggle.objectName: "rtkAutoConnect"
    }

    QGCNoteLabel {
        visible: root._editable && root.configuredBase
        text: root.presentation.automatic
              ? qsTr("If the identified receiver does not support the selected base mode, connecting fails and names the receiver. Auto baud tries every rate the supported receivers use.")
              : !root.presentation.specificReceiver
                ? qsTr("Select a receiver type and its connection to connect manually. Auto baud detects the rate of configurable receivers.")
                : qsTr("Connect only the selected receiver; automatic connections configure it too. A USB adapter's identity only decides which USB ports are tried, not the GNSS manufacturer.")
    }

    GPSReceiverRecording { receiver: root.receiver }

    GPSReceiverFlashConsent {
        receiver: root.receiver
        editable: root._editable
    }

    // Last, below what it connects: Connect, with the receiver's state beside it and its problem below.
    GPSReceiverConnectionRow { receiver: root.receiver }
}
