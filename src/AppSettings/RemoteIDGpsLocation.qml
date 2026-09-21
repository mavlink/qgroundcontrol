import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading:            qsTr("NMEA External GPS")
    Layout.fillWidth:   true
    visible:            !ScreenTools.isMobile
                        && QGroundControl.settingsManager.autoConnectSettings.nmeaSource.userVisible
                        && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.userVisible
                        && root._locationType !== RemoteIDSettings.LocationType.TAKEOFF

    property int    _locationType:    QGroundControl.settingsManager.remoteIDSettings.locationType.value

    readonly property var  _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property var _serialPorts: _serialPortManager ? _serialPortManager.serialPorts : []
    readonly property var _serialBaudRates: _serialPortManager ? _serialPortManager.serialBaudRates : []
    readonly property bool _serialSource: root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceSerial

    LabelledFactComboBox {
        label:              qsTr("Source")
        fact:               root._autoConnectSettings.nmeaSource
        Layout.fillWidth:   true
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
        label:              qsTr("UDP Port")
        fact:               QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
        Layout.fillWidth:   true
        visible:            root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
    }
}
