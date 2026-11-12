pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Serial device and baud rate of the GNSS receiver.
ColumnLayout {
    id: root

    required property Fact deviceFact
    required property Fact baudFact
    /// The ports to offer.
    property list<gpsSerialPortEntry> serialPorts
    /// The explicit rates to offer, which the receiver accepts. Rate 0 is offered before them as "Auto".
    property list<int> serialBaudRates
    /// When set, offers an empty device under this label for consumers that can find the receiver themselves.
    property string anyDeviceLabel
    property bool editable: true

    readonly property string _device: String(deviceFact.rawValue)
    readonly property var _devices: {
        const devices = root.serialPorts.slice()
        if (root.anyDeviceLabel !== "")
            devices.unshift({ value: "", label: root.anyDeviceLabel })
        // The mark comes first, as those of the listed ports do: the picker keeps both ends of a long label.
        if (root._device !== "" && !root._listed(root._device))
            devices.push({ value: root._device, label: qsTr("(unavailable) %1").arg(root._device) })
        if (devices.length === 0)
            devices.push({ value: "", label: qsTr("<none available>") })
        return devices
    }
    readonly property list<int> _rates: [0, ...root.serialBaudRates]
    readonly property int _baudIndex: _rates.indexOf(Number(baudFact.rawValue))
    readonly property bool customBaud: _customRequested || _baudIndex < 0
    readonly property int _baudComboIndex: customBaud ? _rates.length : _baudIndex
    property bool _customRequested: false

    spacing: ScreenTools.defaultFontPixelHeight / 4

    function _listed(device) {
        return root.serialPorts.some(port => port.value === device)
    }

    GPSField {
        label: qsTr("Serial device")

        QGCComboBox {
            objectName: "rtkSerialDevice"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            // A label that is wider than the picker keeps its mark in front and the end of its path.
            labelElide: Text.ElideMiddle
            popupElide: Text.ElideMiddle
            enabled: root.editable && (root.serialPorts.length > 0 || root.anyDeviceLabel !== "")
            model: root._devices
            textRole: "label"
            currentIndex: root._devices.findIndex(device => device.value === root._device)
            onActivated: index => {
                if (index < 0 || index >= root._devices.length)
                    return
                const value = root._devices[index].value
                if (root._listed(value) || (value === "" && root.anyDeviceLabel !== ""))
                    root.deviceFact.rawValue = value
            }
        }
    }

    GPSField {
        label: qsTr("Baud rate")

        QGCComboBox {
            objectName: "rtkSerialBaudRate"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            enabled: root.editable
            model: root._rates.map(rate => rate === 0 ? qsTr("Auto") : String(rate)).concat([qsTr("Custom")])
            currentIndex: root._baudComboIndex
            // A new model resets the index to 0 without re-evaluating the binding, so restore it.
            onModelChanged: currentIndex = Qt.binding(() => root._baudComboIndex)
            onActivated: index => {
                root._customRequested = index === root._rates.length
                if (index >= 0 && index < root._rates.length)
                    root.baudFact.rawValue = root._rates[index]
            }
        }
    }

    GPSField {
        label: qsTr("Custom baud rate")
        visible: root.customBaud

        FactTextField {
            objectName: "rtkCustomBaudRate"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            enabled: root.editable
            fact: root.baudFact
        }
    }

    Connections {
        target: root.baudFact
        function onRawValueChanged() { root._customRequested = false }
    }
}
