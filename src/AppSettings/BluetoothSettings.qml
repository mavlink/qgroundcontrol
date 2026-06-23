pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id:         root

    readonly property var screenTools: ScreenTools
    readonly property var qgc:         QGroundControl
    readonly property var btConfig:    BluetoothConfiguration
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }
    readonly property var palette:     qgcPal

    // Pull shared settings values from LinkSettings Loader when available.
    property var subEditConfig:       (root.parent && root.parent.subEditConfig !== undefined) ? root.parent.subEditConfig : null
    readonly property real _secondColumnWidth: (root.parent && root.parent._secondColumnWidth !== undefined) ? root.parent._secondColumnWidth : (root.screenTools.defaultFontPixelWidth * 30)
    readonly property real _rowSpacing:        (root.parent && root.parent._rowSpacing !== undefined) ? root.parent._rowSpacing : (root.screenTools.defaultFontPixelHeight / 2)
    readonly property real _colSpacing:        (root.parent && root.parent._colSpacing !== undefined) ? root.parent._colSpacing : (root.screenTools.defaultFontPixelWidth / 2)

    spacing:    root._rowSpacing

    visible: root.subEditConfig !== null

    function saveSettings() { }

    //-- Properties --
    property bool paired: false

    readonly property bool   isBleMode:      root.subEditConfig ? root.subEditConfig.mode === root.btConfig.BluetoothMode.ModeLowEnergy : false
    readonly property bool   isClassicMode:  root.subEditConfig ? root.subEditConfig.mode === root.btConfig.BluetoothMode.ModeClassic : false
    readonly property string currentAddress: root.subEditConfig ? root.subEditConfig.address : ""
    readonly property bool   hasAdapter:     root.subEditConfig ? root.subEditConfig.adapterAvailable : false
    readonly property bool   adapterOn:      root.subEditConfig ? root.subEditConfig.adapterPoweredOn : false
    readonly property bool   isScanning:     root.subEditConfig ? root.subEditConfig.scanning : false

    property var knownDevices: []
    property var availableAdapters: []
    property bool _initialAdapterSelected: false

    function toArray(listLike) {
        if (!listLike) {
            return []
        }
        if (Array.isArray(listLike)) {
            return listLike.slice()
        }

        var arr = []
        var length = 0
        if (typeof listLike.length === "number") {
            length = listLike.length
            for (var i = 0; i < length; i++) {
                arr.push(listLike[i])
            }
            return arr
        }
        if (typeof listLike.count === "number" && typeof listLike.get === "function") {
            length = listLike.count
            for (var j = 0; j < length; j++) {
                arr.push(listLike.get(j))
            }
            return arr
        }
        return arr
    }

    // Sorted device list
    readonly property var sortedDevices: {
        if (!root.subEditConfig) return []
        var arr = root.toArray(root.subEditConfig.devicesModel)

        if (root.isBleMode) {
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
        if (!root.subEditConfig) {
            root.knownDevices = []
            root.availableAdapters = []
            _initialAdapterSelected = false
            return
        }
        var connected = root.subEditConfig.getConnectedDevices() || []
        var pairedDevices = root.subEditConfig.getAllPairedDevices() || []
        var seen = {}
        var merged = []

        for (var i = 0; i < connected.length; i++) {
            var dev = connected[i]
            if (dev.address && !seen[dev.address]) {
                seen[dev.address] = true
                merged.push({ name: dev.name, address: dev.address, isConnected: true })
            }
        }
        for (var j = 0; j < pairedDevices.length; j++) {
            var pdev = pairedDevices[j]
            if (pdev.address && !seen[pdev.address]) {
                seen[pdev.address] = true
                merged.push({ name: pdev.name, address: pdev.address, isConnected: false })
            }
        }
        root.knownDevices = merged
        root.availableAdapters = root.subEditConfig.getAllAvailableAdapters() || []

        if (root.subEditConfig.adapterAddress && root.subEditConfig.adapterAddress !== "") {
            _initialAdapterSelected = true
        }

        if (!_initialAdapterSelected && root.availableAdapters.length > 0 && (!root.subEditConfig.adapterAddress || root.subEditConfig.adapterAddress === "")) {
            const firstAddress = root.availableAdapters[0].address || ""
            if (firstAddress !== "") {
                root.subEditConfig.selectAdapter(firstAddress)
                _initialAdapterSelected = (root.subEditConfig.adapterAddress === firstAddress)
            }
        }
    }

    function updatePairingStatus() {
        if (!root.subEditConfig) {
            root.paired = false
            return
        }
        root.paired = root.isClassicMode && root.currentAddress !== "" &&
                 root.subEditConfig.isPaired(root.currentAddress)
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
        target:  root.subEditConfig
        enabled: root.subEditConfig !== null

        function onDevicesModelChanged() { root.refreshDeviceLists(); root.updatePairingStatus() }
        function onDeviceChanged()       { root.updatePairingStatus() }
        function onModeChanged()         { root.refreshDeviceLists(); root.updatePairingStatus() }
        function onAdapterStateChanged() { root.refreshDeviceLists() }
        function onPairingStatusChanged(){ root.refreshDeviceLists(); root.updatePairingStatus() }
        function onErrorOccurred(error)  { root.qgc.showMessageDialog(root, qsTr("Bluetooth Error"), error, Dialog.Ok) }
    }

    Component.onCompleted: {
        refreshDeviceLists()
        updatePairingStatus()
    }

    onSubEditConfigChanged: {
        _initialAdapterSelected = false
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
        columnSpacing:    root._colSpacing
        rowSpacing:       root._rowSpacing
        Layout.fillWidth: true

        QGCLabel {
            text:    qsTr("Adapter")
            visible: root.hasAdapter
        }
        QGCComboBox {
            id:             adapterCombo
            Layout.preferredWidth: root._secondColumnWidth
            visible:        root.hasAdapter
            sizeToContents: false
            clip:           true

            // Column is ~30 chars, MAC + parens = 20 chars, leaves ~10 for name
            readonly property int maxNameLength: 10

            model: {
                var result = []
                for (var i = 0; i < root.availableAdapters.length; i++) {
                    var a = root.availableAdapters[i]
                    var name = a.name || qsTr("Unknown")
                    if (name.length > maxNameLength) {
                        name = name.substring(0, maxNameLength - 1) + "…"
                    }
                    result.push(name + " (" + a.address + ")")
                }
                return result
            }
            currentIndex: {
                var currentAddr = root.subEditConfig ? root.subEditConfig.adapterAddress : ""
                for (var i = 0; i < root.availableAdapters.length; i++) {
                    if (root.availableAdapters[i].address === currentAddr) return i
                }
                return 0
            }
            onActivated: function(index) {
                if (root.subEditConfig && index >= 0 && index < root.availableAdapters.length) {
                    root.subEditConfig.selectAdapter(root.availableAdapters[index].address)
                }
            }
        }

        QGCLabel {
            text:    qsTr("Status")
            visible: root.hasAdapter
        }
        QGCLabel {
            Layout.preferredWidth: root._secondColumnWidth
            text:                  root.subEditConfig ? root.subEditConfig.hostMode : ""
            visible:               root.hasAdapter
            color:                 root.adapterOn ? root.palette.text : root.palette.warningText
        }

        QGCLabel {
            text:              qsTr("Bluetooth adapter unavailable")
            visible:           !root.hasAdapter
            color:             root.palette.warningText
            Layout.columnSpan: 2
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          root._colSpacing
        visible:          root.hasAdapter

        QGCCheckBox {
            text:    qsTr("Powered On")
            checked: root.adapterOn
            onClicked: {
                if (!root.subEditConfig) return
                root.adapterOn ? root.subEditConfig.powerOffAdapter() : root.subEditConfig.powerOnAdapter()
            }
        }

        QGCCheckBox {
            id:      discoverableCheck
            text:    qsTr("Discoverable")
            checked: root.subEditConfig ? root.subEditConfig.adapterDiscoverable : false
            enabled: root.adapterOn
            onClicked: { if (root.subEditConfig) root.subEditConfig.setAdapterDiscoverable(discoverableCheck.checked) }
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
        columnSpacing:    root._colSpacing
        rowSpacing:       root._rowSpacing
        Layout.fillWidth: true

        QGCLabel { text: qsTr("Mode") }
        RowLayout {
            Layout.preferredWidth: root._secondColumnWidth
            spacing:               root._colSpacing

            QGCRadioButton {
                text:    qsTr("Classic")
                checked: root.isClassicMode
                onClicked: { if (root.subEditConfig) root.subEditConfig.mode = root.btConfig.BluetoothMode.ModeClassic }
            }

            QGCRadioButton {
                text:    qsTr("BLE")
                checked: root.isBleMode
                onClicked: { if (root.subEditConfig) root.subEditConfig.mode = root.btConfig.BluetoothMode.ModeLowEnergy }
            }
        }

        QGCLabel { text: qsTr("Selected Device") }
        QGCLabel {
            Layout.preferredWidth: root._secondColumnWidth
            text: root.subEditConfig && root.subEditConfig.deviceName ? root.subEditConfig.deviceName : qsTr("None")
        }

        QGCLabel { text: qsTr("Device Address") }
        QGCLabel {
            Layout.preferredWidth: root._secondColumnWidth
            text: root.currentAddress || qsTr("N/A")
        }

        // Classic Bluetooth Pairing
        QGCLabel {
            text:    qsTr("Pairing")
            visible: root.isClassicMode && root.currentAddress !== ""
        }
        RowLayout {
            Layout.preferredWidth: root._secondColumnWidth
            visible:               root.isClassicMode && root.currentAddress !== ""
            spacing:               root._colSpacing

            QGCLabel {
                text:             root.subEditConfig ? root.subEditConfig.getPairingStatus(root.currentAddress) : ""
                Layout.fillWidth: true
            }

            QGCButton {
                text: root.paired ? qsTr("Unpair") : qsTr("Pair")
                onClicked: {
                    if (!root.subEditConfig) return
                    root.paired ? root.subEditConfig.removePairing(root.currentAddress)
                           : root.subEditConfig.requestPairing(root.currentAddress)
                }
            }
        }

        // BLE Signal Strength
        QGCLabel {
            text:    qsTr("Signal Strength")
            visible: root.isBleMode && (rssiDisplay.hasConnected || rssiDisplay.hasSelected)
        }
        RowLayout {
            id: rssiDisplay
            Layout.preferredWidth: root._secondColumnWidth
            visible: root.isBleMode && (hasConnected || hasSelected)
            spacing: root.screenTools.defaultFontPixelWidth

            readonly property bool hasConnected: root.subEditConfig && root.subEditConfig.connectedRssi !== 0
            readonly property bool hasSelected:  root.subEditConfig && root.subEditConfig.selectedRssi !== 0
            readonly property int  rssi:         hasConnected ? root.subEditConfig.connectedRssi : (root.subEditConfig ? root.subEditConfig.selectedRssi : 0)
            readonly property int  signalLevel:  root.rssiToSignalLevel(rssi)

            Row {
                id: rssiBars
                spacing: 1
                readonly property real barWidth:     root.screenTools.defaultFontPixelWidth * 0.5
                readonly property real maxBarHeight: root.screenTools.defaultFontPixelHeight

                Repeater {
                    model: 4
                    delegate: Item {
                        id: rssiBarDelegate
                        required property int index
                        width:  rssiBars.barWidth
                        height: rssiBars.maxBarHeight

                        Rectangle {
                            width:  parent.width
                            height: root.screenTools.defaultFontPixelHeight * (0.4 + rssiBarDelegate.index * 0.2)
                            color:  rssiBarDelegate.index < rssiDisplay.signalLevel ? root.palette.text : root.palette.buttonText
                            opacity: rssiBarDelegate.index < rssiDisplay.signalLevel ? 1.0 : 0.3
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }

            QGCLabel {
                text: rssiDisplay.rssi + " dBm"
                color: rssiDisplay.hasConnected ? root.palette.text : root.palette.buttonText
            }

            QGCLabel {
                text: rssiDisplay.hasConnected ? qsTr("(Connected)") : qsTr("(Last Scan)")
                font.pointSize: root.screenTools.smallFontPointSize
                color: root.palette.buttonText
            }
        }
    }

    //==========================================================================
    //-- BLE Configuration Section (Collapsible) --
    //==========================================================================
    QGCCheckBox {
        id:      showBleConfig
        text:    qsTr("Advanced BLE Configuration")
        visible: root.isBleMode
        checked: false
    }

    GridLayout {
        columns:          2
        columnSpacing:    root._colSpacing
        rowSpacing:       root._rowSpacing
        Layout.fillWidth: true
        visible:          root.isBleMode && showBleConfig.checked

        QGCLabel { text: qsTr("Service UUID") }
        QGCTextField {
            id:                    serviceUuidField
            Layout.preferredWidth: root._secondColumnWidth
            text:                  root.subEditConfig ? root.subEditConfig.serviceUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (root.subEditConfig) root.subEditConfig.serviceUuid = serviceUuidField.text }
        }

        QGCLabel { text: qsTr("RX Characteristic") }
        QGCTextField {
            id:                    readUuidField
            Layout.preferredWidth: root._secondColumnWidth
            text:                  root.subEditConfig ? root.subEditConfig.readUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (root.subEditConfig) root.subEditConfig.readUuid = readUuidField.text }
        }

        QGCLabel { text: qsTr("TX Characteristic") }
        QGCTextField {
            id:                    writeUuidField
            Layout.preferredWidth: root._secondColumnWidth
            text:                  root.subEditConfig ? root.subEditConfig.writeUuid : ""
            placeholderText:       qsTr("Auto-detect")
            onEditingFinished:     { if (root.subEditConfig) root.subEditConfig.writeUuid = writeUuidField.text }
        }
    }

    QGCLabel {
        text:             qsTr("UUIDs are auto-detected for most devices. Only configure if connection fails.")
        visible:          root.isBleMode && showBleConfig.checked
        wrapMode:         Text.WordWrap
        Layout.fillWidth: true
        font.pointSize:   root.screenTools.smallFontPointSize
        color:            root.palette.buttonText
    }

    //==========================================================================
    //-- Known Devices Section (Classic only) --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text:             qsTr("Known Devices")
        visible:          root.isClassicMode && root.knownDevices.length > 0
    }

    Flow {
        Layout.fillWidth: true
        spacing:          root.screenTools.defaultFontPixelWidth
        visible:          root.isClassicMode && root.knownDevices.length > 0

        Repeater {
            model: root.knownDevices
            delegate: QGCButton {
                required property var modelData
                property var  dev: modelData
                property bool isConnected: dev.isConnected === true

                text: (dev.name || dev.address || qsTr("Unknown")) + (isConnected ? " [Connected]" : "")
                checkable:  true
                checked:    dev.address === root.currentAddress
                onClicked:  { if (root.subEditConfig && dev.address) root.subEditConfig.setDeviceByAddress(dev.address) }
            }
        }
    }

    //==========================================================================
    //-- Available Devices Section --
    //==========================================================================
    SectionHeader {
        Layout.fillWidth: true
        text: root.isBleMode ? qsTr("Available BLE Devices") : qsTr("Available Devices")
    }

    // Scanning status
    RowLayout {
        Layout.fillWidth: true
        visible:          root.isScanning
        spacing:          root.screenTools.defaultFontPixelWidth

        BusyIndicator {
            Layout.preferredWidth:  root.screenTools.defaultFontPixelHeight * 2
            Layout.preferredHeight: Layout.preferredWidth
            running:                true
        }

        QGCLabel {
            text:  qsTr("Scanning for devices...")
            color: root.palette.text
        }
    }

    // Device list
    QGCFlickable {
        id:                     deviceList
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(deviceList.contentHeight, root.screenTools.defaultFontPixelHeight * 16)
        contentHeight:          deviceColumn.height
        contentWidth:           root.width
        clip:                   true
        visible:                !root.isScanning || root.sortedDevices.length > 0

        ColumnLayout {
            id:      deviceColumn
            width:   parent.width
            spacing: root.screenTools.defaultFontPixelHeight * 0.25

            Repeater {
                id:    deviceRepeater
                model: root.sortedDevices
                delegate: QGCButton {
                    id: deviceBtn
                    required property var modelData
                    Layout.fillWidth: true

                    property var    dev:           (typeof modelData === "object") ? modelData : ({ name: modelData })
                    property string deviceName:    dev.name || ""
                    property string deviceAddress: dev.address || ""
                    property bool   hasRssi:       root.isBleMode && (typeof dev.rssi === "number") && dev.rssi !== 0
                    property int    rssiVal:       hasRssi ? dev.rssi : 0
                    property int    signalLevel:   hasRssi ? root.rssiToSignalLevel(rssiVal) : -1
                    property bool   isPaired:      root.isClassicMode && deviceAddress !== "" &&
                                                   root.subEditConfig && root.subEditConfig.isPaired(deviceAddress)

                    checkable:      true
                    autoExclusive:  true
                    checked:        deviceAddress !== "" ? deviceAddress === root.currentAddress
                                                         : deviceName === (root.subEditConfig ? root.subEditConfig.deviceName : "")

                    contentItem: RowLayout {
                        spacing: root.screenTools.defaultFontPixelWidth

                        // Signal bars
                        Row {
                            id: deviceSignalBars
                            visible: deviceBtn.hasRssi
                            spacing: 1
                            readonly property real barWidth:     root.screenTools.defaultFontPixelWidth * 0.4
                            readonly property real maxBarHeight: root.screenTools.defaultFontPixelHeight * 0.9

                            Repeater {
                                model: 4
                                delegate: Item {
                                    id: deviceSignalBarDelegate
                                    required property int index
                                    width:  deviceSignalBars.barWidth
                                    height: deviceSignalBars.maxBarHeight

                                    Rectangle {
                                        width:  parent.width
                                        height: root.screenTools.defaultFontPixelHeight * (0.3 + deviceSignalBarDelegate.index * 0.2)
                                        color:  deviceSignalBarDelegate.index < deviceBtn.signalLevel ? root.palette.text : root.palette.buttonText
                                        opacity: deviceSignalBarDelegate.index < deviceBtn.signalLevel ? 1.0 : 0.3
                                        anchors.bottom: parent.bottom
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            text:             deviceBtn.deviceName || deviceBtn.deviceAddress || qsTr("Unknown")
                            Layout.fillWidth: true
                            elide:            Text.ElideRight
                            color:            deviceBtn.checked ? root.palette.buttonHighlightText : root.palette.buttonText
                        }

                        QGCLabel {
                            visible:        deviceBtn.isPaired
                            text:           qsTr("Paired")
                            font.pointSize: root.screenTools.smallFontPointSize
                            color:          root.palette.buttonText
                        }

                        QGCLabel {
                            visible:        deviceBtn.hasRssi
                            text:           deviceBtn.rssiVal + " dBm"
                            font.pointSize: root.screenTools.smallFontPointSize
                            color:          root.palette.buttonText
                        }
                    }

                    onClicked: {
                        if (!root.subEditConfig) return
                        deviceAddress !== "" ? root.subEditConfig.setDeviceByAddress(deviceAddress)
                                             : root.subEditConfig.setDevice(deviceName)
                    }
                }
            }
        }
    }

    // Empty state
    ColumnLayout {
        Layout.fillWidth: true
        spacing:          root.screenTools.defaultFontPixelHeight
        visible:          !root.isScanning && root.sortedDevices.length === 0

        QGCLabel {
            text:                qsTr("No devices found")
            Layout.fillWidth:    true
            horizontalAlignment: Text.AlignHCenter
            color:               root.palette.warningText
        }

        QGCLabel {
            text: root.isBleMode ? qsTr("Make sure your BLE device is powered on and advertising")
                            : qsTr("Make sure your Bluetooth device is powered on and discoverable")
            Layout.fillWidth:    true
            horizontalAlignment: Text.AlignHCenter
            wrapMode:            Text.WordWrap
            font.pointSize:      root.screenTools.smallFontPointSize
            color:               root.palette.buttonText
        }
    }

    // Scan button
    QGCButton {
        Layout.alignment: Qt.AlignHCenter
        text:             root.isScanning ? qsTr("Stop Scan") : qsTr("Scan for Devices")
        onClicked: {
            if (!root.subEditConfig) return
            root.isScanning ? root.subEditConfig.stopScan() : root.subEditConfig.startScan()
        }
    }
}
