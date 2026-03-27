import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.RTK

/// Connect a GPS receiver to the GCS. The operator picks a single source
/// type (None/NMEA/RTK); only the relevant configuration panel is shown.
/// RTK's base-station config lives in its own section, shown when RTK is
/// connected.
ColumnLayout {
    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var  _rtkSettings:  QGroundControl.settingsManager.gcsGpsSettings
    property var  _gpsMgr:       QGroundControl.gpsManager
    property QtObject _gpsRtk:   QGroundControl.gpsRtk
    property int  _rtkDevCount:  _gpsMgr ? _gpsMgr.deviceCount : 0
    property bool _nmeaActive:   !!_rtkSettings && _rtkSettings.nmeaActive
    property int  _sourceType:   _rtkSettings ? _rtkSettings.positionSourceType.rawValue : 0
    // Enum: 0=None, 1=NMEA, 2=RTK — must match positionSourceType enumValues.
    readonly property int _typeNone: 0
    readonly property int _typeNmea: 1
    readonly property int _typeRtk:  2

    QGCPalette { id: qgcPal }

    // Disconnect any active source that doesn't match the selected type.
    // Runs whenever the operator changes the type combo.
    function _enforceTypeSelection() {
        if (_sourceType !== _typeNmea && _nmeaActive) {
            _rtkSettings.autoConnectNmeaPort.value = "Disabled"
        }
        if (_sourceType !== _typeRtk && _gpsMgr && _rtkDevCount > 0) {
            _gpsMgr.disconnectAll()
        }
        if (_sourceType !== _typeRtk && _rtkSettings.autoConnectRTKGPS.rawValue) {
            _rtkSettings.autoConnectRTKGPS.rawValue = false
        }
    }
    on_SourceTypeChanged: _enforceTypeSelection()

    // Source type selector
    LabelledFactComboBox {
        label:   qsTr("Source")
        fact:    _rtkSettings.positionSourceType
        visible: _rtkSettings.positionSourceType.userVisible
    }

    // -- NMEA GPS ------------------------------------------------------------
    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("NMEA GPS")
        headingDescription: qsTr("Serial or UDP-streamed NMEA position source")
        showDividers:       true
        visible:            _sourceType === _typeNmea

        ConnectionStatusRow {
            statusColor: _nmeaActive ? qgcPal.colorGreen : qgcPal.colorGrey
            statusText: {
                if (!_nmeaActive) return qsTr("Not connected")
                var port = _rtkSettings.autoConnectNmeaPort.valueString
                if (port === "UDP Port")
                    return qsTr("Listening on UDP %1").arg(_rtkSettings.nmeaUdpPort.valueString)
                return qsTr("%1 @ %2").arg(port).arg(_rtkSettings.autoConnectNmeaBaud.valueString)
            }
            buttonText:     qsTr("Disconnect")
            buttonVisible:  _nmeaActive
            onButtonClicked: _rtkSettings.autoConnectNmeaPort.value = "Disabled"
        }

        LabelledComboBox {
            id:      nmeaPortCombo
            label:   qsTr("Device")
            model:   ListModel {}
            visible: !_nmeaActive && _rtkSettings.autoConnectNmeaPort.userVisible

            Component.onCompleted: _rebuildPortModel()

            // Refresh on hot-plug.
            Connections {
                target: QGroundControl.linkManager
                function onCommPortsChanged() { nmeaPortCombo._rebuildPortModel() }
            }

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

                var lastPort = _rtkSettings.autoConnectNmeaPort.valueString
                if (lastPort && lastPort !== "Disabled") {
                    var idx = nmeaPortCombo.comboBox.find(lastPort)
                    if (idx >= 0) nmeaPortCombo.currentIndex = idx
                }
            }
        }

        LabelledComboBox {
            id:      nmeaBaudCombo
            label:   qsTr("Baudrate")
            model:   QGroundControl.linkManager.serialBaudRates
            visible: !_nmeaActive
                     && nmeaPortCombo.currentIndex > 0
                     && QGroundControl.linkManager.serialPorts.length > 0

            Component.onCompleted: {
                var idx = nmeaBaudCombo.comboBox.find(_rtkSettings.autoConnectNmeaBaud.valueString)
                if (idx >= 0) nmeaBaudCombo.currentIndex = idx
            }
        }

        LabelledFactTextField {
            visible: !_nmeaActive && nmeaPortCombo.currentIndex === 0
            label:   qsTr("UDP port")
            fact:    _rtkSettings.nmeaUdpPort
        }

        QGCButton {
            Layout.fillWidth: true
            text:    qsTr("Connect NMEA")
            visible: !_nmeaActive && _rtkSettings.autoConnectNmeaPort.userVisible
            enabled: nmeaPortCombo.currentText !== ""
                     && nmeaPortCombo.currentText !== "Serial <none available>"
            onClicked: {
                if (nmeaPortCombo.currentText !== "UDP Port")
                    _rtkSettings.autoConnectNmeaBaud.value = parseInt(nmeaBaudCombo.currentText)
                _rtkSettings.autoConnectNmeaPort.value = nmeaPortCombo.currentText
            }
        }
    }

    // -- RTK GPS -------------------------------------------------------------
    // (Base-station tuning lives in its own RTK Base Configuration section.)
    property int _manufacturer: _rtkSettings.baseReceiverManufacturers.rawValue
    property int _displayId:    GPSDeviceFlags.All

    function _updateDisplayId() {
        _displayId = GPSDeviceFlags.resolve(_gpsMgr, _manufacturer)
    }
    on_ManufacturerChanged: _updateDisplayId()
    on_RtkDevCountChanged:  _updateDisplayId()
    Component.onCompleted: {
        _updateDisplayId()
        // Reconcile initial state so that on first launch, default values
        // (e.g. autoConnectRTKGPS=true with sourceType=None) don't
        // autoconnect a source the operator hasn't opted into.
        _enforceTypeSelection()
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("RTK GPS")
        headingDescription: qsTr("Local RTK GPS receiver. Base-station behaviour lives under RTK Base Configuration once connected.")
        showDividers:       true
        visible:            _sourceType === _typeRtk

        ConnectionStatusRow {
            statusColor: {
                if (!_gpsMgr) return qgcPal.colorGrey
                if (_gpsRtk && _gpsRtk.valid && _gpsRtk.valid.value) return qgcPal.colorGreen
                if (_rtkDevCount > 0) return qgcPal.colorOrange
                return qgcPal.colorGrey
            }
            statusText: {
                if (!_gpsMgr || _rtkDevCount === 0) return qsTr("Not connected")
                if (_gpsRtk && _gpsRtk.valid && _gpsRtk.valid.value) return qsTr("RTK streaming")
                return qsTr("Connected — survey-in active")
            }
            buttonText:     qsTr("Disconnect")
            buttonVisible:  _rtkDevCount > 0
            onButtonClicked: _gpsMgr.disconnectAll()
        }

        QGCLabel {
            Layout.fillWidth: true
            visible:  _gpsMgr && _gpsMgr.lastError !== ""
            text:     _gpsMgr ? _gpsMgr.lastError : ""
            color:    qgcPal.colorRed
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:    qsTr("AutoConnect RTK GPS")
            fact:    _rtkSettings.autoConnectRTKGPS
            visible: fact.userVisible
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _rtkDevCount === 0 && _rtkSettings.autoConnectRTKGPS.rawValue
            text:    qsTr("Devices matching known RTK receivers will connect automatically when plugged in.")
            font.pointSize: ScreenTools.smallFontPointSize
            color:   qgcPal.colorGrey
            wrapMode: Text.WordWrap
        }

        GridLayout {
            columns:          2
            Layout.fillWidth: true
            columnSpacing:    ScreenTools.defaultFontPixelWidth
            visible:          _rtkDevCount === 0

            QGCLabel { text: qsTr("Receiver") }
            FactComboBox {
                Layout.fillWidth: true
                fact:    _rtkSettings.baseReceiverManufacturers
                visible: fact.userVisible
            }

            QGCLabel { text: qsTr("Serial Port") }
            QGCComboBox {
                id:               rtkPortCombo
                Layout.fillWidth: true
                model:            QGroundControl.linkManager.serialPorts
                currentIndex:     -1
            }
        }

        QGCButton {
            Layout.fillWidth: true
            text:    qsTr("Connect RTK")
            visible: _rtkDevCount === 0 && !_rtkSettings.autoConnectRTKGPS.rawValue
            enabled: rtkPortCombo.currentIndex >= 0
            onClicked: {
                var port = rtkPortCombo.currentText
                var typeStr = _rtkSettings.gpsTypeForManufacturer(_rtkSettings.baseReceiverManufacturers.rawValue)
                _gpsMgr.connectGPS(port, typeStr)
            }
        }
    }
}
