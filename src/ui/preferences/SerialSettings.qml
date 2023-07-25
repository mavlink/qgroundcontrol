/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    spacing: _rowSpacing

    function saveSettings() {
        // No Need
    }

    GridLayout {
        columns:        2
        rowSpacing:     _rowSpacing
        columnSpacing:  _colSpacing

        QGCLabel { text: qsTr("Serial Port") }
        QGCComboBox {
            id:                     commPortCombo
            Layout.preferredWidth:  _secondColumnWidth
            enabled:                QGroundControl.linkManager.serialPorts.length > 0

            onActivated: {
                if (index != -1) {
                    if (index >= QGroundControl.linkManager.serialPortStrings.length) {
                        // This item was adding at the end, must use added text as name
                        subEditConfig.portName = commPortCombo.textAt(index)
                    } else {
                        subEditConfig.portName = QGroundControl.linkManager.serialPorts[index]
                    }
                }
            }

            Component.onCompleted: {
                var index = -1
                var serialPorts = [ ]
                if (QGroundControl.linkManager.serialPortStrings.length !== 0) {
                    for (var i=0; i<QGroundControl.linkManager.serialPortStrings.length; i++) {
                        serialPorts.push(QGroundControl.linkManager.serialPortStrings[i])
                    }
                    if (subEditConfig.portDisplayName === "" && QGroundControl.linkManager.serialPorts.length > 0) {
                        subEditConfig.portName = QGroundControl.linkManager.serialPorts[0]
                    }
                    index = serialPorts.indexOf(subEditConfig.portDisplayName)
                    if (index === -1) {
                        serialPorts.push(subEditConfig.portName)
                        index = serialPorts.indexOf(subEditConfig.portName)
                    }
                }
                if (serialPorts.length === 0) {
                    serialPorts = [ qsTr("None Available") ]
                    index = 0
                }
                commPortCombo.model = serialPorts
                commPortCombo.currentIndex = index
            }
        }

        QGCLabel { text: qsTr("Baud Rate") }
        QGCComboBox {
            id:                     baudCombo
            Layout.preferredWidth:  _secondColumnWidth
            model:                  QGroundControl.linkManager.serialBaudRates

            onActivated: {
                if (index != -1) {
                    subEditConfig.baud = parseInt(QGroundControl.linkManager.serialBaudRates[index])
                }
            }

            Component.onCompleted: {
                var baud = "57600"
                if(subEditConfig != null) {
                    baud = subEditConfig.baud.toString()
                }
                var index = baudCombo.find(baud)
                if (index === -1) {
                    console.warn(qsTr("Baud rate name not in combo box"), baud)
                } else {
                    baudCombo.currentIndex = index
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

        QGCLabel { text: qsTr("Parity") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [qsTr("None"), qsTr("Even"), qsTr("Odd")]

            onActivated: {
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
            currentIndex:           Math.max(Math.min(subEditConfig.dataBits - 5, 0), 3)
            onActivated:            subEditConfig.dataBits = index + 5
        }

        QGCLabel { text: qsTr("Stop Bits") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [ "1", "2" ]
            currentIndex:           Math.max(Math.min(subEditConfig.stopBits - 1, 0), 1)
            onActivated:            subEditConfig.stopBits = index + 1
        }
    }
}
