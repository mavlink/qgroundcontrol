import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    heading:            qsTr("NMEA External GPS")
    Layout.fillWidth:   true
    visible:            !ScreenTools.isMobile
                        && QGroundControl.settingsManager.autoConnectSettings.nmeaSource.userVisible
                        && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.userVisible
                        && _locationType !== RemoteIDSettings.LocationType.TAKEOFF

    property int    _locationType:    QGroundControl.settingsManager.remoteIDSettings.locationType.value

    readonly property var  _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    readonly property bool _serialSource: _autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceSerial

    LabelledFactComboBox {
        label:              qsTr("Source")
        fact:               _autoConnectSettings.nmeaSource
        Layout.fillWidth:   true
    }

    LabelledComboBox {
        id:                 nmeaPortCombo
        label:              qsTr("Device")
        Layout.fillWidth:   true
        visible:            _serialSource

        model: ListModel { }

        onActivated: (index) => {
            if (index !== -1 && QGroundControl.linkManager.serialPorts.length !== 0) {
                _autoConnectSettings.autoConnectNmeaPort.value = comboBox.textAt(index);
            }
        }
        Component.onCompleted: {
            if (QGroundControl.linkManager.serialPorts.length === 0) {
                nmeaPortCombo.model.append({text: qsTr("<none available>")})
            } else {
                for (var i in QGroundControl.linkManager.serialPorts) {
                    nmeaPortCombo.model.append({text: QGroundControl.linkManager.serialPorts[i]})
                }
            }
            var index = nmeaPortCombo.comboBox.find(_autoConnectSettings.autoConnectNmeaPort.valueString);
            nmeaPortCombo.currentIndex = index;
        }
    }

    LabelledComboBox {
        id:                 nmeaBaudCombo
        label:              qsTr("Baudrate")
        Layout.fillWidth:   true
        visible:            _serialSource
        model:              QGroundControl.linkManager.serialBaudRates

        onActivated: (index) => {
            if (index !== -1) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = parseInt(comboBox.textAt(index));
            }
        }
        Component.onCompleted: {
            var index = nmeaBaudCombo.comboBox.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString);
            nmeaBaudCombo.currentIndex = index;
        }
    }

    LabelledFactTextField {
        label:              qsTr("UDP Port")
        fact:               QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
        Layout.fillWidth:   true
        visible:            _autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
    }
}
