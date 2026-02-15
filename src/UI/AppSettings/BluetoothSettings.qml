import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id:         root
    spacing:    _rowSpacing

    visible: subEditConfig !== null

    function saveSettings() { }

    //-- Properties --
    property bool paired: false

    readonly property bool   isBleMode:      subEditConfig ? subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeLowEnergy : false
    readonly property bool   isClassicMode:  subEditConfig ? subEditConfig.mode === BluetoothConfiguration.BluetoothMode.ModeClassic : false
    readonly property string currentAddress: subEditConfig ? subEditConfig.address : ""
    readonly property bool   hasAdapter:     subEditConfig ? subEditConfig.adapterAvailable : false
    readonly property bool   adapterOn:      subEditConfig ? subEditConfig.adapterPoweredOn : false
    readonly property bool   isScanning:     subEditConfig ? subEditConfig.scanning : false

    property var knownDevices: []
    property var availableAdapters: []

    // Sorted device list
    readonly property var sortedDevices: {
        if (!subEditConfig) return []
        var devices = subEditConfig.devicesModel
        if (!devices || devices.length === 0) {
            devices = subEditConfig.nameList || []
        }
        var arr = Array.isArray(devices) ? devices.slice() : []

        if (isBleMode) {
            arr.sort(function(a, b) {
                var rssiA = (typeof a === "object" && a.rssi) ? a.rssi : -999
                var rssiB = (typeof b === "object" && b.rssi) ? b.rssi : -999
                return rssiB - rssiA
            })
        } else {
            arr.sort(function(a, b) {
                var nameA = (typeof a === "object" ? a.name : a) || ""
                var nameB = (typeof b === "object" ? b.name : b) || ""
                return nameA.localeCompare(nameB)
            })
        }
        return arr
    }

    //-- Helper Functions --
    function refreshDeviceLists() {
        if (!subEditConfig) {
            knownDevices = []
            availableAdapters = []
            return
        }
        var connected = subEditConfig.getConnectedDevices() || []
        var paired = subEditConfig.getAllPairedDevices() || []
        var seen = {}
        var merged = []

        for (var i = 0; i < connected.length; i++) {
            var dev = connected[i]
            if (dev.address && !seen[dev.address]) {
                seen[dev.address] = true
                merged.push({ name: dev.name, address: dev.address, isConnected: true })
            }
        }
        for (var j = 0; j < paired.length; j++) {
            var pdev = paired[j]
            if (pdev.address && !seen[pdev.address]) {
                seen[pdev.address] = true
                merged.push({ name: pdev.name, address: pdev.address, isConnected: false })
            }
        }
        knownDevices = merged
        availableAdapters = subEditConfig.getAllAvailableAdapters() || []
    }

    function updatePairingStatus() {
        if (!subEditConfig) {
            paired = false
            return
        }
        paired = isClassicMode && currentAddress !== "" &&
                 subEditConfig.getPairingStatus(currentAddress) !== qsTr("Unpaired")
    }

    function rssiToSignalLevel(rssi) {
        if (rssi >= -50) return 4
        if (rssi >= -60) return 3
        if (rssi >= -70) return 2
        if (rssi >= -80) return 1
        return 0
    }

    //-- Signal Connections --
    Connections {
        target:  subEditConfig
        enabled: subEditConfig !== null

        function onDevicesModelChanged() { refreshDeviceLists(); updatePairingStatus() }
        function onDeviceChanged()       { updatePairingStatus() }
        function onModeChanged()         { refreshDeviceLists(); updatePairingStatus() }
        function onAdapterStateChanged() { refreshDeviceLists() }
        function onPairingStatusChanged(){ refreshDeviceLists(); updatePairingStatus() }
        function onErrorOccurred(error)  { QGroundControl.showMessageDialog(root, qsTr("Bluetooth Error"), error) }
    }

    Component.onCompleted: {
        refreshDeviceLists()
        updatePairingStatus()
    }

    //==========================================================================
    //-- Bluetooth Adapter Section --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text:             qsTr("Bluetooth Adapter")
    }

    GridLayout {
        columns:          2
        columnSpacing:    _colSpacing
        rowSpacing:       _rowSpacing
        Layout.fillWidth: true

        QGCLabel {
            text:    qsTr("Adapter")
            visible: hasAdapter
        }
        QGCComboBox {
            id:             adapterCombo
            width:          _secondColumnWidth
            visible:        hasAdapter
            sizeToContents: false
            clip:           true

            // Column is ~30 chars, MAC + parens = 20 chars, leaves ~10 for name
            readonly property int maxNameLength: 10

            model: {
                var result = []
                for (var i = 0; i < availableAdapters.length; i++) {
                    var a = availableAdapters[i]
                    var name = a.name || qsTr("Unknown")
                    if (name.length > maxNameLength) {
                        name = name.substring(0, maxNameLength - 1) + "…"
                    }
                    result.push(name + " (" + a.address + ")")
                }
                return result
            }
            currentIndex: {
                var currentAddr = subEditConfig ? subEditConfig.adapterAddress : ""
                for (var i = 0; i < availableAdapters.length; i++) {
                    if (availableAdapters[i].address === currentAddr) return i
                }
                return 0
            }
            onActivated: function(index) {
                if (subEditConfig && index >= 0 && index < availableAdapters.length) {
                    subEditConfig.selectAdapter(availableAdapters[index].address)
                }
            }
        }

        QGCLabel {
            text:    qsTr("Status")
            visible: hasAdapter
        }
        QGCLabel {
            Layout.preferredWidth: _secondColumnWidth
            text:                  subEditConfig ? subEditConfig.hostMode : ""
            visible:               hasAdapter
            color:                 adapterOn ? qgcPal.text : qgcPal.warningText
        }

        QGCLabel {
            text:              qsTr("Bluetooth adapter unavailable")
            visible:           !hasAdapter
            color:             qgcPal.warningText
            Layout.columnSpan: 2
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          _colSpacing
        visible:          hasAdapter

        QGCCheckBox {
            text:    qsTr("Powered On")
            checked: adapterOn
            onClicked: {
                if (!subEditConfig) return
                checked ? subEditConfig.powerOnAdapter() : subEditConfig.powerOffAdapter()
            }
        }

        QGCCheckBox {
            text:    qsTr("Discoverable")
            checked: subEditConfig && (subEditConfig.hostMode === qsTr("Discoverable") ||
                                       subEditConfig.hostMode === qsTr("Discoverable (Limited)"))
            enabled: adapterOn
            onClicked: { if (subEditConfig) subEditConfig.setAdapterDiscoverable(checked) }
        }
    }

    //==========================================================================
    //-- Connection Settings Section --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text:             qsTr("Connection")
    }

    GridLayout {
        columns:          2
        columnSpacing:    _colSpacing
        rowSpacing:       _rowSpacing
        Layout.fillWidth: true

        QGCLabel { text: qsTr("Mode") }
        RowLayout {
            Layout.preferredWidth: _secondColumnWidth
            spacing:               _colSpacing

            QGCRadioButton {
                text:    qsTr("Classic")
                checked: isClassicMode
                onClicked: { if (subEditConfig) subEditConfig.mode = BluetoothConfiguration.BluetoothMode.ModeClassic }
            }

            QGCRadioButton {
                text:    qsTr("BLE")
                checked: isBleMode
                onClicked: { if (subEditConfig) subEditConfig.mode = BluetoothConfiguration.BluetoothMode.ModeLowEnergy }
            }
        }

        QGCLabel { text: qsTr("Selected Device") }
        QGCLabel {
            Layout.preferredWidth: _secondColumnWidth
            text: subEditConfig && subEditConfig.deviceName ? subEditConfig.deviceName : qsTr("None")
        }

        QGCLabel { text: qsTr("Device Address") }
        QGCLabel {
            Layout.preferredWidth: _secondColumnWidth
            text: currentAddress || qsTr("N/A")
        }

        // Classic Bluetooth Pairing
        QGCLabel {
            text:    qsTr("Pairing")
            visible: isClassicMode && currentAddress !== ""
        }
        RowLayout {
            Layout.preferredWidth: _secondColumnWidth
            visible:               isClassicMode && currentAddress !== ""
            spacing:               _colSpacing

            QGCLabel {
                text:             subEditConfig ? subEditConfig.getPairingStatus(currentAddress) : ""
                Layout.fillWidth: true
            }

            QGCButton {
                text: paired ? qsTr("Unpair") : qsTr("Pair")
                onClicked: {
                    if (!subEditConfig) return
                    paired ? subEditConfig.removePairing(currentAddress)
                           : subEditConfig.requestPairing(currentAddress)
                }
            }
        }

        // BLE Signal Strength
        QGCLabel {
            text:    qsTr("Signal Strength")
            visible: isBleMode && (rssiDisplay.hasConnected || rssiDisplay.hasSelected)
        }
        RowLayout {
            id: rssiDisplay
            Layout.preferredWidth: _secondColumnWidth
            visible: isBleMode && (hasConnected || hasSelected)
            spacing: ScreenTools.defaultFontPixelWidth

            readonly property bool hasConnected: subEditConfig && subEditConfig.connectedRssi !== 0
            readonly property bool hasSelected:  subEditConfig && subEditConfig.selectedRssi !== 0
            readonly property int  rssi:         hasConnected ? subEditConfig.connectedRssi : (subEditConfig ? subEditConfig.selectedRssi : 0)
            readonly property int  signalLevel:  rssiToSignalLevel(rssi)

            Row {
                spacing: 1
                Repeater {
                    model: 4
                    Rectangle {
                        width:  ScreenTools.defaultFontPixelWidth * 0.5
                        height: ScreenTools.defaultFontPixelHeight * (0.4 + index * 0.2)
                        color:  index < rssiDisplay.signalLevel ? qgcPal.text : qgcPal.buttonText
                        opacity: index < rssiDisplay.signalLevel ? 1.0 : 0.3
                        anchors.bottom: parent.bottom
                    }
                }
            }

            QGCLabel {
                text: rssiDisplay.rssi + " dBm"
                color: rssiDisplay.hasConnected ? qgcPal.text : qgcPal.buttonText
            }

            QGCLabel {
                text: rssiDisplay.hasConnected ? qsTr("(Connected)") : qsTr("(Last Scan)")
                font.pointSize: ScreenTools.smallFontPointSize
                color: qgcPal.buttonText
            }
        }
    }

    //==========================================================================
    //-- BLE Configuration Section (Collapsible) --
    //==========================================================================
    QGCCheckBox {
        id:      showBleConfig
        text:    qsTr("Advanced BLE Configuration")
        visible: isBleMode
        checked: false
    }

    GridLayout {
        columns:          2
        columnSpacing:    _colSpacing
        rowSpacing:       _rowSpacing
        Layout.fillWidth: true
        visible:          isBleMode && showBleConfig.checked

        QGCLabel { text: qsTr("Service UUID") }
        QGCTextField {
            Layout.preferredWidth: _secondColumnWidth
            text:                  subEditConfig ? subEditConfig.serviceUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (subEditConfig) subEditConfig.serviceUuid = text }
        }

        QGCLabel { text: qsTr("RX Characteristic") }
        QGCTextField {
            Layout.preferredWidth: _secondColumnWidth
            text:                  subEditConfig ? subEditConfig.readUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (subEditConfig) subEditConfig.readUuid = text }
        }

        QGCLabel { text: qsTr("TX Characteristic") }
        QGCTextField {
            Layout.preferredWidth: _secondColumnWidth
            text:                  subEditConfig ? subEditConfig.writeUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (subEditConfig) subEditConfig.writeUuid = text }
        }
    }

    QGCLabel {
        text:             qsTr("UUIDs are auto-detected for most devices. Only configure if connection fails.")
        visible:          isBleMode && showBleConfig.checked
        wrapMode:         Text.WordWrap
        Layout.fillWidth: true
        font.pointSize:   ScreenTools.smallFontPointSize
        color:            qgcPal.buttonText
    }

    //==========================================================================
    //-- Known Devices Section (Classic only) --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text:             qsTr("Known Devices")
        visible:          isClassicMode && knownDevices.length > 0
    }

    Flow {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth
        visible:          isClassicMode && knownDevices.length > 0

        Repeater {
            model: knownDevices

            QGCButton {
                property var  dev: modelData
                property bool isConnected: dev.isConnected === true

                text: (dev.name || dev.address || qsTr("Unknown")) + (isConnected ? " [Connected]" : "")
                checkable:  true
                checked:    dev.address === currentAddress
                onClicked:  { if (subEditConfig && dev.address) subEditConfig.setDeviceByAddress(dev.address) }
            }
        }
    }

    //==========================================================================
    //-- Available Devices Section --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text: isBleMode ? qsTr("Available BLE Devices") : qsTr("Available Devices")
    }

    // Scanning status
    RowLayout {
        Layout.fillWidth: true
        visible:          isScanning
        spacing:          ScreenTools.defaultFontPixelWidth

        BusyIndicator {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 2
            Layout.preferredHeight: Layout.preferredWidth
            running:                true
        }

        QGCLabel {
            text:  qsTr("Scanning for devices...")
            color: qgcPal.text
        }
    }

    // Device list
    QGCFlickable {
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 16)
        contentHeight:          deviceColumn.height
        contentWidth:           width
        clip:                   true
        visible:                !isScanning || sortedDevices.length > 0

        ColumnLayout {
            id:      deviceColumn
            width:   parent.width
            spacing: ScreenTools.defaultFontPixelHeight * 0.25

            Repeater {
                id:    deviceRepeater
                model: sortedDevices

                QGCButton {
                    id: deviceBtn
                    Layout.fillWidth: true

                    property var    dev:           (typeof modelData === "object") ? modelData : ({ name: modelData })
                    property string deviceName:    dev.name || ""
                    property string deviceAddress: dev.address || ""
                    property bool   hasRssi:       isBleMode && (typeof dev.rssi === "number") && dev.rssi !== 0
                    property int    rssiVal:       hasRssi ? dev.rssi : 0
                    property int    signalLevel:   hasRssi ? rssiToSignalLevel(rssiVal) : -1
                    property bool   isPaired:      isClassicMode && deviceAddress !== "" &&
                                                   subEditConfig && subEditConfig.getPairingStatus(deviceAddress) !== qsTr("Unpaired")

                    checkable:      true
                    autoExclusive:  true
                    checked:        deviceAddress !== "" ? deviceAddress === currentAddress
                                                         : deviceName === (subEditConfig ? subEditConfig.deviceName : "")

                    contentItem: RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        // Signal bars
                        Row {
                            visible: deviceBtn.hasRssi
                            spacing: 1
                            Repeater {
                                model: 4
                                Rectangle {
                                    width:  ScreenTools.defaultFontPixelWidth * 0.4
                                    height: ScreenTools.defaultFontPixelHeight * (0.3 + index * 0.2)
                                    color:  index < deviceBtn.signalLevel ? qgcPal.text : qgcPal.buttonText
                                    opacity: index < deviceBtn.signalLevel ? 1.0 : 0.3
                                    anchors.bottom: parent.bottom
                                }
                            }
                        }

                        QGCLabel {
                            text:             deviceBtn.deviceName
                            Layout.fillWidth: true
                            elide:            Text.ElideRight
                            color:            deviceBtn.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
                        }

                        QGCLabel {
                            visible:        deviceBtn.isPaired
                            text:           qsTr("Paired")
                            font.pointSize: ScreenTools.smallFontPointSize
                            color:          qgcPal.buttonText
                        }

                        QGCLabel {
                            visible:        deviceBtn.hasRssi
                            text:           deviceBtn.rssiVal + " dBm"
                            font.pointSize: ScreenTools.smallFontPointSize
                            color:          qgcPal.buttonText
                        }
                    }

                    onClicked: {
                        if (!subEditConfig) return
                        deviceAddress !== "" ? subEditConfig.setDeviceByAddress(deviceAddress)
                                             : subEditConfig.setDevice(deviceName)
                    }
                }
            }
        }
    }

    // Empty state
    ColumnLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelHeight
        visible:          !isScanning && sortedDevices.length === 0

        QGCLabel {
            text:                qsTr("No devices found")
            Layout.fillWidth:    true
            horizontalAlignment: Text.AlignHCenter
            color:               qgcPal.warningText
        }

        QGCLabel {
            text: isBleMode ? qsTr("Make sure your BLE device is powered on and advertising")
                            : qsTr("Make sure your Bluetooth device is powered on and discoverable")
            Layout.fillWidth:    true
            horizontalAlignment: Text.AlignHCenter
            wrapMode:            Text.WordWrap
            font.pointSize:      ScreenTools.smallFontPointSize
            color:               qgcPal.buttonText
        }
    }

    // Scan button
    QGCButton {
        Layout.alignment: Qt.AlignHCenter
        text:             isScanning ? qsTr("Stop Scan") : qsTr("Scan for Devices")
        onClicked: {
            if (!subEditConfig) return
            isScanning ? subEditConfig.stopScan() : subEditConfig.startScan()
        }
    }
}
