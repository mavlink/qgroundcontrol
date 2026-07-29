import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    heading: qsTr("NMEA GPS")
    visible: _autoConnectSettings.nmeaSource.userVisible && _autoConnectSettings.autoConnectNmeaBaud.userVisible

    readonly property var  _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    readonly property bool _serialSource: _autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceSerial

    LabelledFactComboBox {
        label: qsTr("Source")
        fact: _autoConnectSettings.nmeaSource
    }

    LabelledComboBox {
        id: nmeaPortCombo
        visible: _serialSource
        label: qsTr("Device")

        model: ListModel {}

        onActivated: (index) => {
            if (index !== -1 && QGroundControl.linkManager.serialPorts.length !== 0) {
                _autoConnectSettings.autoConnectNmeaPort.value = comboBox.textAt(index)
            }
        }

        Component.onCompleted: {
            var model = []

            if (QGroundControl.linkManager.serialPorts.length === 0) {
                model.push(qsTr("<none available>"))
            } else {
                for (var i in QGroundControl.linkManager.serialPorts) {
                    model.push(QGroundControl.linkManager.serialPorts[i])
                }
            }
            nmeaPortCombo.model = model

            nmeaPortCombo.currentIndex = nmeaPortCombo.comboBox.find(_autoConnectSettings.autoConnectNmeaPort.valueString)
        }
    }

    LabelledComboBox {
        id: nmeaBaudCombo
        visible: _serialSource
        label: qsTr("Baudrate")

        readonly property string _customLabel:  qsTr("Custom")
        readonly property bool   isCustomBaud:  currentText === _customLabel

        onActivated: (index) => {
            if (index !== -1 && !isCustomBaud) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = parseInt(comboBox.textAt(index));
            }
        }

        Component.onCompleted: {
            var rates = QGroundControl.linkManager.serialBaudRates.slice()
            rates.push(_customLabel)
            nmeaBaudCombo.model = rates

            var baud = _autoConnectSettings.autoConnectNmeaBaud.valueString
            const index = nmeaBaudCombo.comboBox.find(baud);
            if (index === -1) {
                nmeaBaudCombo.currentIndex = nmeaBaudCombo.comboBox.count - 1
                customNmeaBaudField.text = baud
            } else {
                nmeaBaudCombo.currentIndex = index;
            }
        }
    }

    RowLayout {
        visible: nmeaBaudCombo.visible && nmeaBaudCombo.isCustomBaud
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:               qsTr("Custom Baud Rate")
            Layout.fillWidth:   true
        }
        QGCTextField {
            id:                 customNmeaBaudField
            numericValuesOnly:  true
            validator:          IntValidator { bottom: 1 }
            onEditingFinished: {
                if (!nmeaBaudCombo.isCustomBaud) return
                var baud = parseInt(text)
                if (baud > 0) {
                    _autoConnectSettings.autoConnectNmeaBaud.value = baud
                }
            }
        }
    }

    LabelledFactTextField {
        visible: _autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
        label: qsTr("NMEA stream UDP port")
        fact: _autoConnectSettings.nmeaUdpPort
    }
}
