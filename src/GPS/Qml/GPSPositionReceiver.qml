pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The position receiver: whether one is in use, how to reach it, and what it reports. It is a second receiver that
/// is never configured and only provides the ground station position, so it has a connection and nothing else;
/// GPSReceiverConnection sets up the GNSS receiver.
QGCGroupBox {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.positionReceiver
    property PositionReceiverSettings settings: QGroundControl.settingsManager.positionReceiverSettings
    property list<gpsSerialPortEntry> serialPorts: root.receiver.serialPortEntries
    property list<int> serialBaudRates: root.receiver.serialBaudRates

    readonly property bool _inUse: settings.enabled.rawValue
    // A pending automatic reconnect keeps the saved connection; stop it before editing.
    readonly property bool _editable: receiver.state === GPSReceiver.Disconnected
    readonly property bool _udp: receiver.effectiveConnectionType === RTKSettings.Udp
    readonly property bool _tcp: receiver.effectiveConnectionType === RTKSettings.Tcp
    readonly property bool _serial: receiver.effectiveConnectionType === RTKSettings.Serial
    readonly property bool _bluetooth: receiver.effectiveConnectionType === RTKSettings.Bluetooth

    heading: qsTr("Position Receiver")

    GPSFactSwitchField {
        fact: root.settings.enabled
        note: root.settings.enabled.shortDescription
        toggle.objectName: "positionReceiverEnabled"
    }

    GPSFactComboField {
        fact: root.settings.connectionType
        shown: root._inUse
        comboBox.objectName: "positionReceiverConnectionType"
        comboBox.enabled: root._editable

        QGCNoteLabel {
            objectName: "positionReceiverTcpOnly"
            visible: !root.receiver.serialSupported && root.settings.connectionType.rawValue === RTKSettings.Serial
            text: qsTr("Serial receivers are not supported on this platform, so the receiver connects over TCP.")
        }
    }

    GPSFactTextField {
        objectName: "positionReceiverTcpHost"
        fact: root.settings.tcpHost
        enabled: root._editable
        shown: root._inUse && root._tcp
    }

    GPSFactTextField {
        objectName: "positionReceiverTcpPort"
        fact: root.settings.tcpPort
        enabled: root._editable
        shown: root._inUse && root._tcp
        note: qsTr("Connect to a receiver's TCP port, or to a serial-to-TCP bridge that already runs the receiver link at the receiver's baud rate.")
    }

    GPSFactTextField {
        objectName: "positionReceiverUdpPort"
        fact: root.settings.udpPort
        enabled: root._editable
        shown: root._inUse && root._udp
        note: root.settings.udpPort.shortDescription
    }

    GPSReceiverSerialPort {
        id: serialPort
        objectName: "positionReceiverSerialPort"
        Layout.minimumWidth: 0
        visible: root._inUse && root.receiver.serialSupported && root._serial
        deviceFact: root.settings.serialDevice
        baudFact: root.settings.serialBaudRate
        serialPorts: root.serialPorts
        serialBaudRates: root.serialBaudRates
        editable: root._editable
    }

    GPSReceiverBluetoothDevice {
        objectName: "positionReceiverBluetoothDevice"
        Layout.minimumWidth: 0
        visible: root._inUse && root._bluetooth
        addressFact: root.settings.bluetoothDeviceAddress
        nameFact: root.settings.bluetoothDeviceName
        // Asked for only once a Bluetooth connection is selected, as the receiver creates it then.
        discovery: root._inUse && root._bluetooth ? root.receiver.bluetoothDiscovery : null
        editable: root._editable
        // Nothing is sent to this receiver, so no rate is asked of an adapter but the receiver's own.
        connectionNote: qsTr("The receiver connects over the Bluetooth serial port profile. A Bluetooth serial adapter must already run the receiver link at the receiver's baud rate.")
    }

    QGCNoteLabel {
        objectName: "positionReceiverOutputNote"
        visible: root._inUse && root._editable
        text: qsTr("The receiver's NMEA, u-blox UBX or Septentrio SBF position output is used as it is: set it up with the manufacturer's tool.")
              + (serialPort.visible ? " " + qsTr("Select the receiver's baud rate, or Auto to find the rate by listening, which takes up to 20 seconds.") : "")
    }

    GPSReceiverRecording {
        visible: root._inUse
        receiver: root.receiver
    }

    GPSPositionReceiverStatus {
        receiver: root.receiver
        settings: root.settings
        heading: ""
        showBorder: false
    }
}
