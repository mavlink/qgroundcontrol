import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading: qsTr("NMEA GPS")
    visible: root._autoConnectSettings.nmeaSource.userVisible && root._autoConnectSettings.autoConnectNmeaBaud.userVisible

    readonly property var  _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    property var positionManager: QGroundControl.positionManager
    readonly property var nmeaInput: root.positionManager.nmeaInput
    readonly property var _health: root.nmeaInput ? root.nmeaInput.health : null
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property var _serialPorts: _serialPortManager ? _serialPortManager.serialPorts : []
    readonly property var _serialBaudRates: _serialPortManager ? _serialPortManager.serialBaudRates : []
    readonly property bool _serialSource: root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceSerial

    LabelledFactComboBox {
        label: qsTr("Source")
        fact: root._autoConnectSettings.nmeaSource
    }

    FactSerialPortSettings {
        Layout.fillWidth: true
        visible: root._serialSource
        deviceFact: root._autoConnectSettings.autoConnectNmeaPort
        baudFact: root._autoConnectSettings.autoConnectNmeaBaud
        serialPorts: root._serialPorts
        serialBaudRates: root._serialBaudRates
        deviceObjectName: "nmeaPortCombo"
        baudObjectName: "nmeaBaudCombo"
        customBaudObjectName: "customNmeaBaudField"
    }

    LabelledFactTextField {
        visible: root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
        label: qsTr("NMEA stream UDP port")
        fact: root._autoConnectSettings.nmeaUdpPort
    }

    QGCLabel {
        objectName: "nmeaConnectionStatus"
        visible: root.nmeaInput && (root.nmeaInput.errorMessage.length > 0 || !root._health)
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        textFormat: Text.PlainText
        text: root.nmeaInput ? root.nmeaInput.connectionStatusText : ""
        color: root.nmeaInput && root.nmeaInput.errorMessage.length > 0
               ? QGroundControl.globalPalette.warningText : QGroundControl.globalPalette.text
    }

    QGCLabel {
        visible: root._health !== null
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: root.nmeaInput && root.nmeaInput.receiving ? qsTr("Receiving NMEA data")
              : root.nmeaInput && root.nmeaInput.hasData ? qsTr("NMEA stream idle")
              : root._serialSource ? qsTr("Waiting for NMEA data") : qsTr("Listening for NMEA UDP data")
    }

    QGCLabel {
        visible: root._health !== null
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: root._health && root._health.usable ? qsTr("Position usable") : qsTr("Waiting for a usable fix")
    }

    QGCLabel {
        visible: root._health !== null
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Satellites: %1 in use, %2 in view")
            .arg(root._health && root._health.satellitesInUseCount >= 0 ? root._health.satellitesInUseCount : qsTr("Unknown"))
            .arg(root._health && root._health.satellitesInViewCount >= 0 ? root._health.satellitesInViewCount : qsTr("Unknown"))
    }
}
