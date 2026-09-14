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
    readonly property var _serialPortManager: QGroundControl.serialPortManager
    readonly property var _serialPorts: _serialPortManager ? _serialPortManager.serialPorts : []
    readonly property var _serialBaudRates: _serialPortManager ? _serialPortManager.serialBaudRates : []
    readonly property bool _serialSource: root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceSerial

    LabelledFactComboBox {
        label: qsTr("Source")
        fact: root._autoConnectSettings.nmeaSource
    }

    LabelledComboBox {
        id: nmeaPortCombo
        objectName: "nmeaPortCombo"
        visible: root._serialSource
        label: qsTr("Device")

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
        id: nmeaBaudCombo
        objectName: "nmeaBaudCombo"
        visible: root._serialSource
        label: qsTr("Baudrate")

        readonly property string _customLabel:  qsTr("Custom")
        readonly property bool   isCustomBaud:  currentText === _customLabel

        onActivated: (index) => {
            if (index !== -1 && !isCustomBaud) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = parseInt(comboBox.textAt(index));
            }
        }

        Component.onCompleted: {
            var rates = root._serialBaudRates.slice()
            rates.push(_customLabel)
            nmeaBaudCombo.model = rates

            var baud = root._autoConnectSettings.autoConnectNmeaBaud.valueString
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
            objectName:         "customNmeaBaudField"
            numericValuesOnly:  true
            validator:          IntValidator { bottom: 1 }
            onEditingFinished: {
                if (!nmeaBaudCombo.isCustomBaud) return
                var baud = parseInt(text)
                if (baud > 0) {
                    root._autoConnectSettings.autoConnectNmeaBaud.value = baud
                }
            }
        }
    }

    LabelledFactTextField {
        visible: root._autoConnectSettings.nmeaSource.rawValue === AutoConnectSettings.NmeaSourceUdp
        label: qsTr("NMEA stream UDP port")
        fact: root._autoConnectSettings.nmeaUdpPort
    }
}
