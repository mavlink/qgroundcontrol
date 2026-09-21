pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: root

    required property Fact deviceFact
    required property Fact baudFact
    property var serialPorts: []
    property var serialBaudRates: []
    property int minimumBaud: 1
    property int maximumBaud: 4000000
    property bool editable: true
    property string deviceObjectName: "serialDevice"
    property string baudObjectName: "serialBaudRate"
    property string customBaudObjectName: "customSerialBaudRate"

    readonly property string _device: String(deviceFact.rawValue)
    readonly property var _devices: {
        const devices = root.serialPorts.map(port => ({ value: port, label: port }))
        if (root._device !== "" && !root.serialPorts.includes(root._device))
            devices.push({ value: root._device, label: qsTr("%1 (unavailable)").arg(root._device) })
        if (devices.length === 0)
            devices.push({ value: "", label: qsTr("<none available>") })
        return devices
    }
    readonly property var _rates: root.serialBaudRates.map(Number).filter((rate, index, rates) =>
        Number.isInteger(rate) && rate >= root.minimumBaud && rate <= root.maximumBaud
        && rates.indexOf(rate) === index)
    readonly property int _baudIndex: _rates.indexOf(Number(baudFact.rawValue))
    readonly property bool customBaud: _customRequested || _baudIndex < 0
    property bool _customRequested: false

    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("Serial device")
    }
    QGCComboBox {
        objectName: root.deviceObjectName
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.editable && root.serialPorts.length > 0
        model: root._devices
        textRole: "label"
        currentIndex: root._devices.findIndex(device => device.value === root._device)
        onActivated: index => {
            if (index >= 0 && index < root._devices.length
                && root.serialPorts.includes(root._devices[index].value))
                root.deviceFact.rawValue = root._devices[index].value
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("Baud rate")
    }
    QGCComboBox {
        objectName: root.baudObjectName
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        enabled: root.editable
        readonly property bool isCustomBaud: root.customBaud
        model: root._rates.map(rate => String(rate)).concat([qsTr("Custom")])
        currentIndex: root.customBaud ? root._rates.length : root._baudIndex
        onActivated: index => {
            root._customRequested = index === root._rates.length
            if (index >= 0 && index < root._rates.length)
                root.baudFact.rawValue = root._rates[index]
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        visible: root.customBaud
        text: qsTr("Custom baud rate")
    }
    FactTextField {
        objectName: root.customBaudObjectName
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: root.customBaud
        enabled: root.editable
        fact: root.baudFact
    }

    Connections {
        target: root.baudFact
        function onRawValueChanged() { root._customRequested = false }
    }
}
