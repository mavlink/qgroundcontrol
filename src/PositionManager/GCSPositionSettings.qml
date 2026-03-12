import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    id: root
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var  _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    property var  _posMgr:              QGroundControl.qgcPositionManger
    property bool _nmeaActive:          _autoConnectSettings.autoConnectNmeaPort.valueString !== ""
                                        && _autoConnectSettings.autoConnectNmeaPort.valueString !== "Disabled"

    QGCPalette { id: qgcPal }

    // -- Position Status -----------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Position Status")
        headingDescription: qsTr("Ground station location used on the map and as GGA source for NTRIP")
        showDividers:       true

        ConnectionStatusRow {
            statusColor: {
                if (_posMgr && _posMgr.gcsPosition.isValid) return qgcPal.colorGreen
                if (_nmeaActive) return qgcPal.colorOrange
                return qgcPal.colorGrey
            }
            statusText: {
                if (!_posMgr) return qsTr("Unavailable")
                if (_posMgr.gcsPosition.isValid) return qsTr("Position Valid")
                if (_nmeaActive) return qsTr("Waiting for position data...")
                return qsTr("No position source configured")
            }
            buttonText:    qsTr("Disconnect")
            buttonVisible: _nmeaActive
            onButtonClicked: _autoConnectSettings.autoConnectNmeaPort.value = "Disabled"
        }

        LabelledLabel {
            label:     qsTr("Source")
            labelText: {
                var port = _autoConnectSettings.autoConnectNmeaPort.valueString
                if (port === "UDP Port")
                    return qsTr("UDP Port %1").arg(_autoConnectSettings.nmeaUdpPort.valueString)
                if (port !== "" && port !== "Disabled")
                    return qsTr("Serial: %1 @ %2").arg(port).arg(_autoConnectSettings.autoConnectNmeaBaud.valueString)
                return qsTr("None")
            }
            visible: _nmeaActive
        }

        LabelledLabel {
            label:     qsTr("Latitude / Longitude")
            labelText: {
                if (!_posMgr || !_posMgr.gcsPosition.isValid) return "-.--"
                return _posMgr.gcsPosition.latitude.toFixed(7) + ", " + _posMgr.gcsPosition.longitude.toFixed(7)
            }
            visible:   _posMgr && _posMgr.gcsPosition.isValid
        }

        LabelledLabel {
            label:     qsTr("Horizontal Accuracy")
            labelText: {
                if (!_posMgr || !isFinite(_posMgr.gcsPositionHorizontalAccuracy)) return "-.--"
                return _posMgr.gcsPositionHorizontalAccuracy.toFixed(1) + " m"
            }
            visible:   _posMgr && isFinite(_posMgr.gcsPositionHorizontalAccuracy)
        }

        LabelledLabel {
            label:     qsTr("Heading")
            labelText: {
                if (!_posMgr || isNaN(_posMgr.gcsHeading)) return "-.--"
                return _posMgr.gcsHeading.toFixed(1) + "\u00B0"
            }
            visible:   _posMgr && !isNaN(_posMgr.gcsHeading)
        }
    }

    // -- NMEA GPS Device -----------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("NMEA GPS Device")
        headingDescription: qsTr("Connect an external NMEA GPS receiver via serial port or UDP")
        showDividers:       true
        visible:            !_nmeaActive
                            && _autoConnectSettings.autoConnectNmeaPort.visible
                            && _autoConnectSettings.autoConnectNmeaBaud.visible

        LabelledComboBox {
            id:    nmeaPortCombo
            label: qsTr("Device")
            model: ListModel {}

            Component.onCompleted: _rebuildPortModel()

            function _rebuildPortModel() {
                var portModel = []
                portModel.push(qsTr("UDP Port"))
                if (QGroundControl.linkManager.serialPorts.length === 0) {
                    portModel.push(qsTr("Serial <none available>"))
                } else {
                    for (var i in QGroundControl.linkManager.serialPorts)
                        portModel.push(QGroundControl.linkManager.serialPorts[i])
                }
                nmeaPortCombo.model = portModel

                // Pre-select previously used port
                var lastPort = _autoConnectSettings.autoConnectNmeaPort.valueString
                if (lastPort && lastPort !== "Disabled") {
                    var idx = nmeaPortCombo.comboBox.find(lastPort)
                    if (idx >= 0) nmeaPortCombo.currentIndex = idx
                }
            }
        }

        LabelledComboBox {
            id:      nmeaBaudCombo
            visible: nmeaPortCombo.currentText !== "UDP Port"
                     && nmeaPortCombo.currentText !== "Serial <none available>"
            label:   qsTr("Baudrate")
            model:   QGroundControl.linkManager.serialBaudRates

            Component.onCompleted: {
                var idx = nmeaBaudCombo.comboBox.find(_autoConnectSettings.autoConnectNmeaBaud.valueString)
                if (idx >= 0) nmeaBaudCombo.currentIndex = idx
            }
        }

        LabelledFactTextField {
            visible: nmeaPortCombo.currentText === "UDP Port"
            label:   qsTr("NMEA stream UDP port")
            fact:    _autoConnectSettings.nmeaUdpPort
        }

        QGCButton {
            Layout.fillWidth: true
            text:    qsTr("Connect")
            enabled: nmeaPortCombo.currentText !== ""
                     && nmeaPortCombo.currentText !== "Serial <none available>"
            onClicked: {
                if (nmeaPortCombo.currentText !== "UDP Port") {
                    _autoConnectSettings.autoConnectNmeaBaud.value = parseInt(nmeaBaudCombo.currentText)
                }
                _autoConnectSettings.autoConnectNmeaPort.value = nmeaPortCombo.currentText
            }
        }
    }
}
