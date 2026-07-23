import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    heading: qsTr("NMEA GPS")
    visible: QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.userVisible && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.userVisible

    LabelledComboBox {
        id: nmeaPortCombo
        label: qsTr("Device")

        model: ListModel {}

        // Untranslated value stored in the setting for each entry, parallel to the display model.
        // LinkManager compares these against fixed strings, so they must not be translated.
        property var _values: []

        onActivated: (index) => {
            if (index !== -1) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = nmeaPortCombo._values[index];
            }
        }

        Component.onCompleted: {
            var model = []
            var values = []

            model.push(qsTr("Disabled"))
            values.push("Disabled")

            model.push(qsTr("UDP Port"))
            values.push("UDP Port")

            if (QGroundControl.linkManager.serialPorts.length === 0) {
                model.push(qsTr("Serial <none available>"))
                values.push("Serial <none available>")
            } else {
                for (var i in QGroundControl.linkManager.serialPorts) {
                    model.push(QGroundControl.linkManager.serialPorts[i])
                    values.push(QGroundControl.linkManager.serialPorts[i])
                }
            }
            nmeaPortCombo._values = values
            nmeaPortCombo.model = model

            const savedValue = QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString
            var index = values.indexOf(savedValue)
            if (index === -1) {
                // Settings written by an older build stored the translated label. Match the
                // displayed text instead and rewrite the setting to the untranslated value.
                index = nmeaPortCombo.comboBox.find(savedValue)
                if (index !== -1) {
                    QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = values[index]
                }
            }
            nmeaPortCombo.currentIndex = index
        }
    }

    LabelledComboBox {
        id: nmeaBaudCombo
        visible: nmeaPortCombo.currentIndex > 1
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

            var baud = QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString
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
                    QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = baud
                }
            }
        }
    }

    LabelledFactTextField {
        visible: nmeaPortCombo.currentIndex === 1
        label: qsTr("NMEA stream UDP port")
        fact: QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
    }
}
