pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// The Bluetooth device of the GNSS receiver: the saved device, and the devices a scan finds to choose from.
ColumnLayout {
    id: root

    required property Fact addressFact
    required property Fact nameFact
    /// The device discovery of the application's Bluetooth links, a BluetoothConfiguration: the picker lists its
    /// devicesModel and scans with its startScan() and stopScan(). Null where receivers do not connect over Bluetooth.
    /// The pickers of both receivers scan with the same one, so each shows the scan the other started.
    property var discovery: null
    property bool editable: true
    /// QGroundControl configures the receiver, at a rate the link cannot be set to: a Bluetooth serial adapter in
    /// front of the receiver has to run at that rate already. A receiver that is only listened to has its own.
    property bool configured: true

    readonly property string _address: String(addressFact.rawValue)
    readonly property string _name: String(nameFact.rawValue)
    readonly property bool _scanning: discovery ? discovery.scanning : false
    /// The devices the scan found, by name, without the saved one.
    readonly property var _found: {
        const found = []
        const listed = root.discovery ? root.discovery.devicesModel : []
        for (let i = 0; i < listed.length; ++i) {
            const address = String(listed[i].address || "")
            if (address !== "" && address.toUpperCase() !== root._address.toUpperCase())
                found.push({ address: address, name: String(listed[i].name || "") })
        }
        found.sort((a, b) => a.name.localeCompare(b.name) || a.address.localeCompare(b.address))
        return found
    }
    /// The saved device comes first, whether or not a scan lists it: it may be off or out of range now. Without one,
    /// an entry that selects nothing takes its place.
    readonly property var _devices: [{ address: root._address, name: root._name }, ...root._found]
    /// Why the last scan failed; empty once another one starts.
    property string _scanError
    /// This picker started the scan that runs, or asked for one that has not started yet.
    property bool _ownsScan: false

    spacing: ScreenTools.defaultFontPixelHeight / 4

    function _label(device) {
        if (device.address === "")
            return qsTr("<none selected>")
        //: %1 is the name of a Bluetooth device and %2 its address
        return device.name !== "" ? qsTr("%1 – %2").arg(device.name).arg(device.address) : device.address
    }

    /// Ends the scan at the user's word, whichever picker started it.
    function _stopScan() {
        if (root._scanning)
            root.discovery.stopScan()
    }

    /// Ends the scan this picker started. Another picker's is not this one's to end when it goes or is locked.
    function _stopOwnScan() {
        if (root._ownsScan)
            root._stopScan()
    }

    GPSField {
        label: qsTr("Bluetooth device")

        QGCComboBox {
            objectName: "rtkBluetoothDevice"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            enabled: root.editable && root._found.length > 0
            model: root._devices.map(device => root._label(device))
            currentIndex: 0
            onActivated: index => {
                if (index < 0 || index >= root._devices.length || root._devices[index].address === "")
                    return
                const device = root._devices[index]
                root._stopScan()
                // The name first: the address is what selects another connection.
                root.nameFact.rawValue = device.name
                root.addressFact.rawValue = device.address
            }
        }

        QGCButton {
            objectName: "rtkBluetoothScan"
            text: root._scanning ? qsTr("Stop Scan") : qsTr("Scan for Devices")
            enabled: root.editable && root.discovery !== null
            onClicked: {
                if (root._scanning) {
                    root.discovery.stopScan()
                } else {
                    root._scanError = ""
                    root._ownsScan = true
                    root.discovery.startScan()
                }
            }
        }

        QGCNoteLabel {
            objectName: "rtkBluetoothScanError"
            visible: text !== ""
            warning: true
            text: root._scanError
        }

        QGCNoteLabel {
            objectName: "rtkBluetoothNote"
            text: root._scanning
                  ? qsTr("Scanning for Bluetooth devices...")
                  : root._address === ""
                    ? qsTr("No Bluetooth device is selected. Pair the receiver with this device, then scan and select it.")
                    : root.configured
                      ? qsTr("The receiver connects over the Bluetooth serial port profile. A Bluetooth serial adapter must already run the receiver link at 115200 baud; QGroundControl cannot change an adapter's rate.")
                      : qsTr("The receiver connects over the Bluetooth serial port profile. A Bluetooth serial adapter must already run the receiver link at the receiver's baud rate.")
        }
    }

    Connections {
        target: root.discovery
        function onErrorOccurred(error) {
            root._scanError = error
            root._ownsScan = false
        }
        function onScanningChanged() {
            // Asked of the discovery: the binding that follows it may not have run yet.
            if (root.discovery.scanning)
                root._scanError = ""
            else
                root._ownsScan = false
        }
    }

    // A scan is for choosing a device: it ends when there is nothing to choose, and with the picker.
    onEditableChanged: {
        if (!editable)
            _stopOwnScan()
    }
    onVisibleChanged: {
        if (!visible)
            _stopOwnScan()
    }
    Component.onDestruction: _stopOwnScan()
}
