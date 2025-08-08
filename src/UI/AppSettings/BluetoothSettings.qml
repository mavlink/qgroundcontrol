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

    // UUID validation regex: accepts empty (auto-detect), standard UUID (with/without hyphens), or short 4-digit UUID
    readonly property var uuidValidator: RegularExpressionValidator {
        regularExpression: /^$|^(0x)?[0-9a-fA-F]{4}$|^[0-9a-fA-F]{32}$|^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/
    }

    // Property to track whether selected device is paired
    property bool paired: false

    // Update pairing status when device list or selection changes
    Connections {
        target: subEditConfig
        function onDevicesModelChanged() {
            updatePairingStatus()
        }
        function onDeviceChanged() {
            updatePairingStatus()
        }
        function onModeChanged() {
            updatePairingStatus()
        }
    }

    // Helper function to update pairing status for selected device
    function updatePairingStatus() {
        if (subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic && subEditConfig.address !== "") {
            paired = subEditConfig.getPairingStatus(subEditConfig.address) !== qsTr("Unpaired")
        } else {
            paired = false
        }
    }

    // Initialize pairing status on load
    Component.onCompleted: updatePairingStatus()

    //-- Bluetooth Adapter Status --
    SectionHeader {
        Layout.fillWidth: true
        text: qsTr("Bluetooth Adapter")
    }

    GridLayout {
        columns:            2
        columnSpacing:      _colSpacing
        rowSpacing:         _rowSpacing
        Layout.fillWidth:   true

        QGCLabel {
            text:       qsTr("Adapter")
            visible:    subEditConfig.adapterAvailable
        }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.adapterName + " (" + subEditConfig.adapterAddress + ")"
            visible:                subEditConfig.adapterAvailable
        }

        QGCLabel {
            text:       qsTr("Status")
            visible:    subEditConfig.adapterAvailable
        }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.hostMode
            visible:                subEditConfig.adapterAvailable
            color:                  subEditConfig.adapterPoweredOn ? qgcPal.text : qgcPal.warningText
        }

        QGCLabel {
            text:               qsTr("Bluetooth adapter unavailable")
            visible:            !subEditConfig.adapterAvailable
            color:              qgcPal.warningText
            Layout.columnSpan:  2
        }
    }

    GridLayout {
        columns:            2
        columnSpacing:      _colSpacing
        rowSpacing:         _rowSpacing
        Layout.fillWidth:   true
        visible:            subEditConfig.adapterAvailable

        QGCCheckBox {
            text:       qsTr("Powered On")
            checked:    subEditConfig.adapterPoweredOn
            onClicked:  {
                if (checked) {
                    subEditConfig.powerOnAdapter()
                } else {
                    subEditConfig.powerOffAdapter()
                }
            }
        }

        QGCCheckBox {
            text:       qsTr("Discoverable")
            checked:    subEditConfig.hostMode === qsTr("Discoverable") || subEditConfig.hostMode === qsTr("Discoverable (Limited)")
            enabled:    subEditConfig.adapterPoweredOn
            onClicked:  subEditConfig.setAdapterDiscoverable(checked)
        }
    }

    //-- Connection Settings --
    SectionHeader {
        Layout.fillWidth: true
        text: qsTr("Connection")
    }

    GridLayout {
        columns:            2
        columnSpacing:      _colSpacing
        rowSpacing:         _rowSpacing
        Layout.fillWidth:   true

        QGCLabel { text: qsTr("Mode") }
        RowLayout {
            Layout.preferredWidth: _secondColumnWidth
            spacing: _colSpacing

            QGCRadioButton {
                text:       qsTr("Classic")
                checked:    subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic
                onClicked:  subEditConfig.mode = BluetoothConfiguration.BluetoothMode.ModeClassic
            }

            QGCRadioButton {
                text:       qsTr("BLE")
                checked:    subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy
                onClicked:  subEditConfig.mode = BluetoothConfiguration.BluetoothMode.ModeLowEnergy
            }
        }

        QGCLabel { text: qsTr("Selected Device") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.deviceName || qsTr("None")
        }

        QGCLabel { text: qsTr("Device Address") }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.address || qsTr("N/A")
        }

        // Classic Bluetooth Pairing
        QGCLabel {
            text:       qsTr("Pairing")
            visible:    subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic && subEditConfig.address !== ""
        }
        RowLayout {
            Layout.preferredWidth:  _secondColumnWidth
            visible:                subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic && subEditConfig.address !== ""
            spacing:                _colSpacing

            QGCLabel {
                text:               (paired, subEditConfig.getPairingStatus(subEditConfig.address))
                Layout.fillWidth:   true
            }

            QGCButton {
                text:       paired ? qsTr("Unpair") : qsTr("Pair")
                onClicked:  {
                    if (paired) {
                        subEditConfig.removePairing(subEditConfig.address)
                    } else {
                        subEditConfig.requestPairing(subEditConfig.address)
                    }
                }
            }
        }

        // BLE Signal Strength
        QGCLabel {
            text:       qsTr("Signal Strength")
            visible:    subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy &&
                        (((typeof subEditConfig.connectedRssi === "number") && subEditConfig.connectedRssi !== 0) ||
                         ((typeof subEditConfig.selectedRssi  === "number") && subEditConfig.selectedRssi  !== 0))
        }
        QGCLabel {
            Layout.preferredWidth:  _secondColumnWidth
            readonly property bool hasConnected: (typeof subEditConfig.connectedRssi === "number") && subEditConfig.connectedRssi !== 0
            readonly property bool hasSelected:  (typeof subEditConfig.selectedRssi  === "number") && subEditConfig.selectedRssi  !== 0
            visible:                subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy && (hasConnected || hasSelected)
            text:                   hasConnected ? qsTr("%1 dBm (Connected)").arg(subEditConfig.connectedRssi)
                                                 : qsTr("%1 dBm (Last Scan)").arg(subEditConfig.selectedRssi)
            color:                  hasConnected ? qgcPal.text : qgcPal.buttonText
        }
    }

    //-- BLE Configuration --
    SectionHeader {
        Layout.fillWidth:   true
        text:               qsTr("BLE Configuration")
        visible:            subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy
    }

    GridLayout {
        columns:            2
        columnSpacing:      _colSpacing
        rowSpacing:         _rowSpacing
        Layout.fillWidth:   true
        visible:            subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy

        QGCLabel { text: qsTr("Service UUID") }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   subEditConfig.serviceUuid
            placeholderText:        qsTr("Auto-detect")
            validator:              uuidValidator
            onTextChanged:          subEditConfig.serviceUuid = text
        }

        QGCLabel {
            text:       qsTr("RX Characteristic")
            visible:    typeof subEditConfig.readUuid !== "undefined"
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   typeof subEditConfig.readUuid !== "undefined" ? subEditConfig.readUuid : ""
            placeholderText:        qsTr("Auto-detect")
            visible:                typeof subEditConfig.readUuid !== "undefined"
            validator:              uuidValidator
            onTextChanged:          { if (typeof subEditConfig.readUuid !== "undefined") subEditConfig.readUuid = text }
        }

        QGCLabel {
            text:       qsTr("TX Characteristic")
            visible:    typeof subEditConfig.writeUuid !== "undefined"
        }
        QGCTextField {
            Layout.preferredWidth:  _secondColumnWidth
            text:                   typeof subEditConfig.writeUuid !== "undefined" ? subEditConfig.writeUuid : ""
            placeholderText:        qsTr("Auto-detect")
            visible:                typeof subEditConfig.writeUuid !== "undefined"
            validator:              uuidValidator
            onTextChanged:          { if (typeof subEditConfig.writeUuid !== "undefined") subEditConfig.writeUuid = text }
        }
    }

    QGCLabel {
        text:               qsTr("Common Service UUIDs:\n  • Nordic UART: 6e400001-b5a3-f393-e0a9-e50e24dcca9e\n  • TI SensorTag: 0000ffe0-0000-1000-8000-00805f9b34fb")
        visible:            subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy
        wrapMode:           Text.WordWrap
        Layout.fillWidth:   true
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.buttonText
    }

    //-- Available Devices --
    SectionHeader {
        Layout.fillWidth: true
        text: subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy ? qsTr("Available BLE Devices") : qsTr("Available Devices")
    }

    ScrollView {
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(repeaterColumn.height, ScreenTools.defaultFontPixelHeight * 12)
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
                    readonly property string address: _dev.address || ""
                    readonly property bool   hasRssi: (typeof _dev.rssi === "number") && _dev.rssi !== 0
                    readonly property int    rssi: hasRssi ? _dev.rssi : 0
                    // Use paired as dependency to force re-evaluation when any pairing changes
                    readonly property bool   isPaired: (paired === paired) &&
                                                         subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic &&
                                                         address !== "" &&
                                                         subEditConfig.getPairingStatus(address) !== qsTr("Unpaired")

                    text: {
                        let displayText = deviceName
                        if (hasRssi) {
                            displayText += "  (" + rssi + " dBm)"
                        }
                        if (isPaired) {
                            displayText += " 🔗"
                        }
                        return displayText
                    }
                    width:          _secondColumnWidth
                    checkable:      true
                    autoExclusive:  true
                    checked:        (address !== "") ? (address === subEditConfig.address) : (deviceName === subEditConfig.deviceName)

                    onClicked: {
                        if (address !== "") {
                            subEditConfig.setDeviceByAddress(address)
                        } else if (deviceName !== "") {
                            subEditConfig.setDevice(deviceName)
                        }
                    }
                }
            }

            QGCLabel {
                text:                   qsTr("No devices found")
                visible:                (subEditConfig.devicesModel ? subEditConfig.devicesModel.length === 0 : subEditConfig.nameList.length === 0) && !subEditConfig.scanning
                width:                  _secondColumnWidth
                horizontalAlignment:    Text.AlignHCenter
                color:                  qgcPal.warningText
            }

            BusyIndicator {
                visible:    subEditConfig.scanning
                running:    visible
                width:      _secondColumnWidth
                height:     ScreenTools.defaultFontPixelHeight * 2
            }
        }
    }

    QGCButton {
        Layout.alignment:   Qt.AlignHCenter
        text:               subEditConfig.scanning ? qsTr("Stop Scan") : qsTr("Scan for Devices")
        onClicked:          {
            if (subEditConfig.scanning) {
                subEditConfig.stopScan()
            } else {
                subEditConfig.startScan()
            }
        }
    }
}
