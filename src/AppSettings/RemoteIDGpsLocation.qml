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

    LabelledComboBox {
        id:                 nmeaPortCombo
        objectName:         "nmeaPortCombo"
        label:              qsTr("Device")
        Layout.fillWidth:   true
        visible:            root._serialSource

        model: root._serialPorts.length > 0 ? root._serialPorts : [qsTr("<none available>")]
        currentIndex: root._serialPorts.length > 0
                      ? root._serialPorts.indexOf(root._autoConnectSettings.autoConnectNmeaPort.valueString) : 0
        enabled: root._serialPorts.length > 0

        onActivated: (index) => {
            if (index >= 0 && index < root._serialPorts.length) {
                root._autoConnectSettings.autoConnectNmeaPort.value = root._serialPorts[index]
            }
        }
    }

    LabelledComboBox {
        id:                 nmeaBaudCombo
        objectName:         "nmeaBaudCombo"
        label:              qsTr("Baudrate")
        Layout.fillWidth:   true
        visible:            root._serialSource
        model:              root._serialBaudRates
        currentIndex:       root._serialBaudRates.indexOf(root._autoConnectSettings.autoConnectNmeaBaud.valueString)

        onActivated: (index) => {
            if (index >= 0 && index < root._serialBaudRates.length) {
                root._autoConnectSettings.autoConnectNmeaBaud.value = parseInt(root._serialBaudRates[index])
            }
        }
    }

    LabelledFactTextField {
        label:              qsTr("UDP Port")
        fact:               QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
        Layout.fillWidth:   true
        visible:            root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
    }
}
