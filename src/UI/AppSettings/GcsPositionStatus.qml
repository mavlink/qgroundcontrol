import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Read-only GCS position dashboard. Connect UI is in GCS GPS Receivers.
SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("GCS Position")
    headingDescription: qsTr("Ground station location used on the map and as GGA source for NTRIP")
    showDividers:       true

    property var  _posMgr:     QGroundControl.qgcPositionManager
    property var  _gpsMgr:     QGroundControl.gpsManager
    property var  _gcsGpsSettings: QGroundControl.settingsManager.gcsGpsSettings
    property bool _nmeaActive: !!_gcsGpsSettings && _gcsGpsSettings.nmeaActive
    property bool _rtkActive:  !!_gpsMgr && _gpsMgr.deviceCount > 0
    property bool _hasSource:  _rtkActive || _nmeaActive

    QGCPalette { id: qgcPal }

    ConnectionStatusRow {
        statusColor: {
            if (_hasSource && _posMgr && _posMgr.gcsPosition.isValid) return qgcPal.colorGreen
            if (_hasSource) return qgcPal.colorOrange
            return qgcPal.colorGrey
        }
        statusText: {
            if (!_posMgr) return qsTr("Unavailable")
            if (!_hasSource) return qsTr("No position source configured")
            var src = _posMgr.sourceDescription
            if (_posMgr.gcsPosition.isValid)
                return src ? qsTr("Position Valid (%1)").arg(src) : qsTr("Position Valid")
            if (_rtkActive)  return qsTr("RTK GPS connected, waiting for fix...")
            if (_nmeaActive) return qsTr("Waiting for NMEA position data...")
            return qsTr("No position source configured")
        }
        buttonVisible: false
    }

    LabelledLabel {
        label:     qsTr("Source")
        labelText: {
            var parts = []
            if (_rtkActive) {
                var dev = _gpsMgr.devices.count > 0 ? _gpsMgr.devices.get(0) : null
                parts.push(dev && dev.devicePath ? qsTr("RTK GPS: %1").arg(dev.devicePath)
                                                 : qsTr("RTK GPS"))
            }
            if (_nmeaActive) {
                var port = _gcsGpsSettings.autoConnectNmeaPort.valueString
                if (port === "UDP Port")
                    parts.push(qsTr("NMEA UDP %1").arg(_gcsGpsSettings.nmeaUdpPort.valueString))
                else
                    parts.push(qsTr("NMEA Serial: %1 @ %2").arg(port).arg(_gcsGpsSettings.autoConnectNmeaBaud.valueString))
            }
            return parts.length > 0 ? parts.join(" · ") : qsTr("None")
        }
        visible: _hasSource
    }

    LabelledLabel {
        label:     qsTr("Latitude / Longitude")
        labelText: {
            if (!_posMgr || !_posMgr.gcsPosition.isValid) return "-.--"
            return GPSFormatter.formatCoordinate(_posMgr.gcsPosition.latitude, _posMgr.gcsPosition.longitude)
        }
        visible: _hasSource && Boolean(_posMgr) && _posMgr.gcsPosition.isValid
    }

    LabelledLabel {
        label:     qsTr("Horizontal Accuracy")
        labelText: _posMgr ? GPSFormatter.formatAccuracy(_posMgr.gcsPositionHorizontalAccuracy) : "-.--"
        visible: _hasSource && Boolean(_posMgr) && isFinite(_posMgr.gcsPositionHorizontalAccuracy)
    }

    LabelledLabel {
        label:     qsTr("Heading")
        labelText: {
            if (!_posMgr || isNaN(_posMgr.gcsHeading)) return "-.--"
            return GPSFormatter.formatHeading(_posMgr.gcsHeading)
        }
        visible: _hasSource && Boolean(_posMgr) && !isNaN(_posMgr.gcsHeading)
    }
}
