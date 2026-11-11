pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.GPS

/// Which receiver to connect and how to reach it: its role, family, connection and device.
GPSReceiverSettingsGroup {
    id: root

    property list<gpsSerialPortEntry> serialPorts: root.receiver.serialPortEntries
    property list<int> serialBaudRates: root.receiver.serialBaudRates

    readonly property int _connection: settings.connectionType.rawValue
    readonly property bool _udp: receiver.effectiveConnectionType === RTKSettings.Udp
    readonly property bool _tcp: receiver.effectiveConnectionType === RTKSettings.Tcp
    readonly property bool _serial: receiver.effectiveConnectionType === RTKSettings.Serial
    readonly property bool _connectionSupported: receiver.connectionSupported

    heading: qsTr("GNSS Receiver")

    GPSFactComboField {
        fact: root.settings.receiverRole
        comboBox.objectName: "rtkReceiverRole"
        comboBox.enabled: root._editable
        note: root.configuredBase
              ? qsTr("QGroundControl configures a supported receiver as an RTK base station and forwards its RTCM corrections to vehicles.")
              : qsTr("QGroundControl never configures this receiver. Its NMEA, u-blox UBX or Septentrio SBF position output provides the ground station position. Configure the receiver's output externally and select its existing baud rate. No survey-in status is inferred.")
    }

    GPSFactSwitchField {
        fact: root.settings.forwardReceiverRtcm
        note: root.settings.forwardReceiverRtcm.shortDescription
        shown: !root.configuredBase
        toggle.objectName: "rtkForwardReceiverRtcm"
        toggle.enabled: root._editable
    }

    GPSFactComboField {
        fact: root.settings.baseReceiverManufacturer
        shown: root.configuredBase
        comboBox.objectName: "rtkManufacturer"
        comboBox.enabled: root._editable

        GPSNoteLabel {
            objectName: "rtkAutomaticExplanation"
            visible: root.presentation.automatic
            text: root.settings.baseReceiverManufacturer.shortDescription
        }
    }

    GPSFactComboField {
        fact: root.settings.connectionType
        comboBox.objectName: "rtkConnectionType"
        comboBox.enabled: root._editable

        GPSNoteLabel {
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
        note: qsTr("Connect to a receiver's TCP port or a serial-to-TCP bridge. A bridge must already run the receiver link at 115200 baud; QGroundControl cannot change a bridge's rate.")
    }

    GPSFactTextField {
        objectName: "rtkUdpPort"
        fact: root.settings.udpPort
        enabled: root._editable
        shown: root._udp

        GPSNoteLabel {
            objectName: "rtkUdpExplanation"
            warning: !root._connectionSupported
            text: root._connectionSupported ? root.settings.udpPort.shortDescription
                                            : qsTr("A configured base needs a serial or TCP connection. UDP only receives data.")
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
        allowAutoBaud: !root.presentation.passive
        editable: root._editable
    }

    GPSNoteLabel {
        visible: root._editable && root.configuredBase
        text: root.presentation.automatic
              ? qsTr("If the identified receiver does not support the selected base mode, connecting fails and names the receiver. Auto baud tries every rate the supported receivers use.")
              : !root.presentation.specificReceiver
                ? qsTr("Select a receiver type and its connection to connect manually. Auto baud detects the rate of configurable receivers.")
                : qsTr("Connect only the selected receiver; automatic connections configure it too. A USB adapter's identity only decides which USB ports are tried, not the GNSS manufacturer.")
    }
}
