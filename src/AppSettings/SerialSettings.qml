import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    property var linkManager: QGroundControl.linkManager

    // Whether the selected port can be set to hardware flow control. An Android USB adapter whose driver has none
    // refuses to open with it, so a link to such a port is not left with it.
    property bool _flowControlAvailable: true

    // Where ports are device files, a port can also be typed as a path that names it for good, such as
    // /dev/serial/by-id/..., which outlasts the ttyACMn name the port gets this time it is plugged in
    readonly property bool _devicePathsSupported: Qt.platform.os === "linux" || Qt.platform.os === "osx"

    spacing: _rowSpacing

    function saveSettings() {
        if (baudCombo.isCustomBaud) {
            var baud = parseInt(customBaudField.text)
            if (baud > 0) {
                subEditConfig.baud = baud
            }
        }
        commPortCombo.applyEditText()
    }

    function _refreshFlowControl() {
        if (!subEditConfig) {
            return
        }
        _flowControlAvailable = linkManager.serialPortHasFlowControl(subEditConfig.portName)
        if (!_flowControlAvailable && subEditConfig.flowControl !== 0) {
            subEditConfig.flowControl = 0
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
            editable:               _devicePathsSupported
            sizeToContents:         true    // so that the popup shows the whole of a long by-id path
            textRole:               "label"
            valueRole:              "path"

            // The field that an editable combo box is typed into
            contentItem: T.TextField {
                implicitWidth:      contentWidth
                implicitHeight:     contentHeight
                text:               commPortCombo.editable ? commPortCombo.editText : commPortCombo.displayText
                enabled:            commPortCombo.editable
                autoScroll:         commPortCombo.editable
                readOnly:           commPortCombo.down
                selectByMouse:      true
                inputMethodHints:   Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                font:               commPortCombo.font
                color:              portPal.textPrimary
                selectionColor:     portPal.accent
                selectedTextColor:  portPal.textPrimary
                verticalAlignment:  Text.AlignVCenter
                onEditingFinished:  commPortCombo.applyEditText()

                QGCPalette { id: portPal; colorGroupEnabled: commPortCombo.enabled }
            }

            function refreshPorts() {
                const paths = linkManager.serialPorts
                const labels = linkManager.serialPortStrings
                const links = editable ? linkManager.serialPortLinks : []
                const entries = []
                let selectedPath = subEditConfig.portName
                if (!selectedPath && paths.length > 0) {
                    // A new link takes the first port by a link to it where there is one, for a name that lasts
                    const link = editable ? linkManager.serialPortLinkTo(paths[0]) : ""
                    selectedPath = link || paths[0]
                    subEditConfig.portName = selectedPath
                }
                // First, as the names to prefer, which outlast a replug; shown as the path, which is what the link
                // keeps when one is chosen
                for (const link of links) {
                    if (paths.indexOf(link) < 0) {
                        entries.push({ label: link, path: link })
                    }
                }
                for (let i = 0; i < paths.length; ++i) {
                    entries.push({ label: labels[i] || paths[i], path: paths[i] })
                }
                let selectedIndex = entries.findIndex(entry => entry.path === selectedPath)
                if (selectedIndex < 0 && selectedPath) {
                    entries.push({ label: selectedPath, path: selectedPath })
                    selectedIndex = entries.length - 1
                }
                if (entries.length === 0) {
                    entries.push({ label: qsTr("None Available"), path: "" })
                    selectedIndex = 0
                }
                // A port appearing or going while a path is typed does not throw the typing away
                const typed = (editable && contentItem.activeFocus) ? editText : null
                model = entries
                currentIndex = selectedIndex
                if (typed !== null) {
                    editText = typed
                }
            }

            /// Has the link use the port typed in: the listed port of that name, or else the text as a path.
            function applyEditText() {
                if (!editable || !enabled || !subEditConfig) {
                    return
                }
                const text = editText.trim()
                if (text === "") {
                    return
                }
                const index = find(text)
                const path = (index >= 0) ? model[index].path : text
                if (path && path !== subEditConfig.portName) {
                    subEditConfig.portName = path
                }
            }

            onActivated: (index) => {
                if (index >= 0 && index < model.length && model[index].path) {
                    subEditConfig.portName = model[index].path
                }
            }

            onAccepted: applyEditText()

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
            objectName:         "flowControlCheckBox"
            text:               qsTr("Enable Flow Control")
            enabled:            _flowControlAvailable
            checked:            subEditConfig ? subEditConfig.flowControl !== 0 : false
            onCheckedChanged:   { if (subEditConfig) subEditConfig.flowControl = checked ? 1 : 0 }
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
            currentIndex:           subEditConfig ? Math.max(Math.min(subEditConfig.dataBits - 5, 3), 0) : -1
            onActivated: (index) => { subEditConfig.dataBits = index + 5 }
        }

        QGCLabel { text: qsTr("Stop Bits") }
        QGCComboBox {
            Layout.preferredWidth:  _secondColumnWidth
            model:                  [ "1", "2" ]
            currentIndex:           subEditConfig ? Math.max(Math.min(subEditConfig.stopBits - 1, 1), 0) : -1
            onActivated: (index) => { subEditConfig.stopBits = index + 1 }
        }
    }

    Connections {
        target: linkManager
        function onCommPortsChanged() { _refreshFlowControl() }
    }
    Connections {
        target: subEditConfig
        function onPortNameChanged() { _refreshFlowControl() }
    }

    Component.onCompleted: _refreshFlowControl()
}
