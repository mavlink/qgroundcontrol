/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: _rowSpacing

    function saveSettings() { }

    GridLayout {
        columns:    2
        columnSpacing:  _colSpacing
        rowSpacing:     _rowSpacing

        QGCLabel { text: qsTr("Mode") }
        RowLayout {
            Layout.preferredWidth: _secondColumnWidth
            spacing: _colSpacing

            QGCRadioButton {
                text: qsTr("Classic")
                checked: subEditConfig.mode === 0  // ModeClassic = 0
                onClicked: subEditConfig.mode = 0
            }

            QGCRadioButton {
                text: qsTr("BLE")
                checked: subEditConfig.mode === 1  // ModeLowEnergy = 1
                onClicked: subEditConfig.mode = 1
            }
        }

        QGCLabel { text: qsTr("Device") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.deviceName || qsTr("None selected")
        }

        QGCLabel { text: qsTr("Address") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.address || qsTr("N/A")
        }

        QGCLabel {
            text:       qsTr("Signal (RSSI)")
            visible:    typeof subEditConfig.connectedRssi === "number"
        }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   typeof subEditConfig.connectedRssi === "number" ? (subEditConfig.connectedRssi + " dBm") : qsTr("N/A")
            visible:                typeof subEditConfig.connectedRssi === "number"
        }

        // BLE-specific settings
        QGCLabel {
            text: qsTr("Service UUID")
            visible: subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.serviceUuid
            placeholderText:        qsTr("Auto-detect")
            visible:                subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy
            onTextChanged:          subEditConfig.serviceUuid = text
        }

        QGCLabel {
            text:       qsTr("Read Characteristic UUID")
            visible:    subEditConfig.mode === 1 && typeof subEditConfig.readUuid !== "undefined"
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   typeof subEditConfig.readUuid !== "undefined" ? subEditConfig.readUuid : ""
            placeholderText:        qsTr("Auto-detect")
            visible:                subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy && typeof subEditConfig.readUuid !== "undefined"
            onTextChanged:          { if (typeof subEditConfig.readUuid !== "undefined") subEditConfig.readUuid = text }
        }

        QGCLabel {
            text:       qsTr("Write Characteristic UUID")
            visible:    subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy && typeof subEditConfig.writeUuid !== "undefined"
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   typeof subEditConfig.writeUuid !== "undefined" ? subEditConfig.writeUuid : ""
            placeholderText:        qsTr("Auto-detect")
            visible:                subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy && typeof subEditConfig.writeUuid !== "undefined"
            onTextChanged:          { if (typeof subEditConfig.writeUuid !== "undefined") subEditConfig.writeUuid = text }
        }
    }

    QGCLabel { text: subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy ? qsTr("BLE Devices") : qsTr("Bluetooth Devices") }

    ScrollView {
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(repeaterColumn.height, ScreenTools.defaultFontPixelHeight * 10)
        clip:                   true

        Column {
            id:         repeaterColumn
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelHeight * 0.5

            Repeater {
                model: subEditConfig.devicesModel && subEditConfig.devicesModel.length
                       ? subEditConfig.devicesModel
                       : subEditConfig.nameList

                delegate: QGCButton {
                    readonly property var _dev: (typeof modelData === "object") ? modelData : ({ name: modelData })
                    readonly property string deviceName: _dev.name || ""
                    readonly property bool   hasRssi: typeof _dev.rssi === "number"
                    readonly property int    rssi: hasRssi ? _dev.rssi : 0

                    text:                   hasRssi ? (deviceName + "  (" + rssi + " dBm)") : deviceName
                    width:                  _secondColumnWidth
                    checkable:              true
                    autoExclusive:          true
                    checked:                deviceName === subEditConfig.deviceName

                    onClicked: {
                        if (deviceName !== "") {
                            subEditConfig.setDevice(deviceName)
                        }
                    }
                }
            }

            QGCLabel {
                text:       qsTr("No devices found")
                visible:    (subEditConfig.devicesModel ? subEditConfig.devicesModel.length === 0 : subEditConfig.nameList.length === 0) && !subEditConfig.scanning
                width:      _secondColumnWidth
                horizontalAlignment: Text.AlignHCenter
                color:      qgcPal.warningText
            }

            BusyIndicator {
                visible:    subEditConfig.scanning
                running:    visible
                width:      _secondColumnWidth
                height:     ScreenTools.defaultFontPixelHeight * 2
            }
        }
    }

    RowLayout {
        Layout.alignment:   Qt.AlignCenter
        spacing:            _colSpacing

        QGCButton {
            text:       qsTr("Scan")
            enabled:    !subEditConfig.scanning
            onClicked:  subEditConfig.startScan()
        }

        QGCButton {
            text:       qsTr("Stop")
            enabled:    subEditConfig.scanning
            onClicked:  subEditConfig.stopScan()
        }
    }

    QGCLabel {
        text:               qsTr("Note: BLE devices require service UUID for connection")
        visible:            subEditConfig.mode === 1
        wrapMode:           Text.WordWrap
        Layout.fillWidth:   true
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.warningText
    }
}
