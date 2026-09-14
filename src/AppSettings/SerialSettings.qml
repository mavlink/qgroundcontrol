import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    property var linkManager: QGroundControl.linkManager

    spacing: _rowSpacing

    function saveSettings() {
        if (baudCombo.isCustomBaud) {
            var baud = parseInt(customBaudField.text)
            if (baud > 0) {
                subEditConfig.baud = baud
            }
        }
    }

    GridLayout {
        columns:        2
        rowSpacing:     _rowSpacing
        columnSpacing:  _colSpacing

        QGCLabel { text: qsTr("Serial Port") }
        QGCComboBox {
            id:                     commPortCombo
            Layout.preferredWidth:  _secondColumnWidth
            objectName:             "serialPortCombo"
            enabled:                linkManager.serialPorts.length > 0
            textRole:               "label"
            valueRole:              "path"

            function refreshPorts() {
                const paths = linkManager.serialPorts
                const labels = linkManager.serialPortStrings
                const entries = []
                let selectedPath = subEditConfig.portName
                if (!selectedPath && paths.length > 0) {
                    selectedPath = paths[0]
                    subEditConfig.portName = selectedPath
                }
                for (let i = 0; i < paths.length; ++i) {
                    entries.push({ label: labels[i] || paths[i], path: paths[i] })
                }
                let selectedIndex = paths.indexOf(selectedPath)
                if (selectedIndex < 0 && selectedPath) {
                    entries.push({ label: selectedPath, path: selectedPath })
                    selectedIndex = entries.length - 1
                }
                if (entries.length === 0) {
                    entries.push({ label: qsTr("None Available"), path: "" })
                    selectedIndex = 0
                }
                model = entries
                currentIndex = selectedIndex
            }

            onActivated: (index) => {
                if (index >= 0 && index < model.length && model[index].path) {
                    subEditConfig.portName = model[index].path
                }
            }

            Component.onCompleted: refreshPorts()

            Connections {
                target: linkManager
                function onCommPortsChanged() { commPortCombo.refreshPorts() }
                function onCommPortStringsChanged() { commPortCombo.refreshPorts() }
            }
            Connections {
                target: subEditConfig
                function onPortNameChanged() { commPortCombo.refreshPorts() }
            }
        }

        QGCLabel { text: qsTr("Baud Rate") }
        QGCComboBox {
            id:                     baudCombo
            Layout.preferredWidth:  _secondColumnWidth

            readonly property string _customLabel:    qsTr("Custom")
            readonly property bool   isCustomBaud:    currentText === _customLabel

            onActivated: (index) => {
                if (index !== -1 && !isCustomBaud) {
                    subEditConfig.baud = parseInt(currentText)
                }
            }

            Component.onCompleted: {
                var rates = linkManager.serialBaudRates.slice()
                rates.push(_customLabel)
                model = rates

                var baud = subEditConfig ? subEditConfig.baud.toString() : "57600"
                var index = baudCombo.find(baud)
                if (index === -1) {
                    baudCombo.currentIndex = baudCombo.count - 1
                    customBaudField.text = baud
                } else {
                    baudCombo.currentIndex = index
                }
            }
        }

        QGCLabel {
            text:    qsTr("Custom Baud Rate")
            visible: baudCombo.isCustomBaud
        }
        QGCTextField {
            id:                     customBaudField
            Layout.preferredWidth:  _secondColumnWidth
            visible:                baudCombo.isCustomBaud
            numericValuesOnly:      true
            validator:              IntValidator { bottom: 1 }
            onEditingFinished: {
                if (!baudCombo.isCustomBaud) return
                var baud = parseInt(text)
                if (baud > 0) {
                    subEditConfig.baud = baud
                }
            }
        }
    }

    QGCCheckBox {
        id:         advancedSettings
        text:       qsTr("Advanced Settings")
        checked:    false
    }

    GridLayout {
        columns:        2
        rowSpacing:     _rowSpacing
        columnSpacing:  _colSpacing
        visible:        advancedSettings.checked

        QGCCheckBox {
            Layout.columnSpan:  2
            text:               qsTr("Enable Flow Control")
            checked:            subEditConfig.flowControl !== 0
            onCheckedChanged:   subEditConfig.flowControl = checked ? 1 : 0
        }

        QGCCheckBox {
            Layout.columnSpan:  2
            text:               qsTr("Force DTR Low")
            checked:            subEditConfig ? subEditConfig.dtrForceLow : false
            onCheckedChanged:   { if (subEditConfig) subEditConfig.dtrForceLow = checked }
        }

        QGCLabel { text: qsTr("Parity") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [qsTr("None"), qsTr("Even"), qsTr("Odd")]

            onActivated: (index) => {
                // Hard coded values from qserialport.h
                switch (index) {
                case 0:
                    subEditConfig.parity = 0
                    break
                case 1:
                    subEditConfig.parity = 2
                    break
                case 2:
                    subEditConfig.parity = 3
                    break
                }
            }

            Component.onCompleted: {
                switch (subEditConfig.parity) {
                case 0:
                    currentIndex = 0
                    break
                case 2:
                    currentIndex = 1
                    break
                case 3:
                    currentIndex = 2
                    break
                default:
                    console.warn("Unknown parity", subEditConfig.parity)
                    break
                }
            }
        }

        QGCLabel { text: qsTr("Data Bits") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [ "5", "6", "7", "8" ]
            currentIndex:           Math.max(Math.min(subEditConfig.dataBits - 5, 3), 0)
            onActivated: (index) => { subEditConfig.dataBits = index + 5 }
        }

        QGCLabel { text: qsTr("Stop Bits") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [ "1", "2" ]
            currentIndex:           Math.max(Math.min(subEditConfig.stopBits - 1, 1), 0)
            onActivated: (index) => { subEditConfig.stopBits = index + 1 }
        }
    }
}
