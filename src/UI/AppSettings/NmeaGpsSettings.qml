import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    heading: qsTr("NMEA GPS")
    visible: QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.visible && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.visible

    LabelledComboBox {
        id: nmeaPortCombo
        label: qsTr("Device")

        model: ListModel {}

        onActivated: (index) => {
            if (index !== -1) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = comboBox.textAt(index);
            }
        }

        Component.onCompleted: {
            var model = []

            model.push(qsTr("Disabled"))
            model.push(qsTr("UDP Port"))

            if (QGroundControl.linkManager.serialPorts.length === 0) {
                model.push(qsTr("Serial <none available>"))
            } else {
                for (var i in QGroundControl.linkManager.serialPorts) {
                    model.push(QGroundControl.linkManager.serialPorts[i])
                }
            }
            nmeaPortCombo.model = model

            const index = nmeaPortCombo.comboBox.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString);
            nmeaPortCombo.currentIndex = index;
        }
    }

    LabelledComboBox {
        id: nmeaBaudCombo
        visible: (nmeaPortCombo.currentText !== "UDP Port") && (nmeaPortCombo.currentText !== "Disabled")
        label: qsTr("Baudrate")
        model: QGroundControl.linkManager.serialBaudRates

        onActivated: (index) => {
            if (index !== -1) {
                QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = parseInt(comboBox.textAt(index));
            }
        }

        Component.onCompleted: {
            const index = nmeaBaudCombo.comboBox.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString);
            nmeaBaudCombo.currentIndex = index;
        }
    }

    LabelledFactTextField {
        visible: nmeaPortCombo.currentText === "UDP Port"
        label: qsTr("NMEA stream UDP port")
        fact: QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
    }
}
