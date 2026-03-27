import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

SettingsGroupLayout {
    id: root

    required property var device

    property var _fg:  device ? device.factGroup : null
    property var _sat: device ? device.satelliteModel : null
    property var _rtkSettings: QGroundControl.settingsManager.gcsGpsSettings
    property string _na:      qsTr("N/A")
    property string _valueNA: qsTr("-.--")

    heading:      device ? qsTr("RTK Base: %1").arg(device.deviceType) : qsTr("RTK Base Station")
    showDividers: true
    visible:      device ? (device.connected === true) : false

    function _hasFact(name) { return !!_fg && _fg[name] !== undefined }
    function _factVal(name) { return _hasFact(name) ? _fg[name].value : undefined }

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        FixStatusDot {
            statusColor: device && device.lastError ? qgcPal.colorRed
                         : _hasFact("valid") && _fg.valid.value ? qgcPal.colorGreen
                         : _hasFact("active") && _fg.active.value ? qgcPal.colorOrange
                         : _hasFact("connected") && _fg.connected.value ? qgcPal.colorGreen
                         : qgcPal.colorGrey
        }

        QGCLabel {
            text: {
                if (device && device.lastError) return qsTr("Error")
                if (_hasFact("valid") && _fg.valid.value) return qsTr("RTK Streaming")
                if (_hasFact("active") && _fg.active.value) return qsTr("Survey-in Active")
                if (_hasFact("connected") && _fg.connected.value) return qsTr("Connected")
                return qsTr("Connecting")
            }
        }
    }

    QGCLabel {
        text:           device ? device.lastError : ""
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorRed
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        device && device.lastError !== ""
        Layout.fillWidth: true
    }

    LabelledLabel {
        label:     qsTr("Device")
        labelText: device && device.devicePath !== undefined ? device.devicePath : _na
    }

    LabelledLabel {
        label:     qsTr("Fix Type")
        labelText: _hasFact("baseFixType") ? (_fg.baseFixType.enumStringValue || _na) : _na
        visible:   _hasFact("baseFixType") && _fg.baseFixType.rawValue > 0
    }

    LabelledLabel {
        label:     qsTr("Satellites")
        labelText: _hasFact("numSatellites") ? _fg.numSatellites.valueString : _na
    }

    LabelledLabel {
        label:     qsTr("Constellations")
        labelText: _sat ? _sat.constellationSummary : ""
        visible:   !!_sat && _sat.constellationSummary !== undefined && _sat.constellationSummary !== ""
    }

    LabelledLabel {
        label:     qsTr("Duration")
        labelText: _hasFact("currentDuration") ? GPSFormatter.formatDuration(_fg.currentDuration.value) : _na
        visible:   _factVal("currentDuration") > 0
    }

    LabelledLabel {
        label:     _factVal("valid") ? qsTr("Accuracy") : qsTr("Current Accuracy")
        labelText: _hasFact("currentAccuracy") ? GPSFormatter.formatAccuracy(_fg.currentAccuracy.value) : _valueNA
        visible:   _factVal("currentAccuracy") > 0
    }

    ColumnLayout {
        id:      surveyProgressCol
        spacing: 2
        visible: Boolean(_factVal("active")) && !_factVal("valid") && _factVal("currentAccuracy") > 0
        Layout.fillWidth: true

        property real _targetAcc: _rtkSettings ? _rtkSettings.surveyInAccuracyLimit.rawValue : 1.0
        property real _progress:  _fg && _fg.currentAccuracy && _targetAcc > 0 && _fg.currentAccuracy.value > 0
                                  ? Math.min(1.0, _targetAcc / _fg.currentAccuracy.value)
                                  : 0

        QGCLabel {
            text:           qsTr("Survey-in Progress: %1%").arg(Math.round(surveyProgressCol._progress * 100))
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Rectangle {
            Layout.fillWidth: true
            height: ScreenTools.defaultFontPixelHeight * 0.4
            color:  qgcPal.window
            border.color: qgcPal.text
            border.width: 1
            radius: height / 2

            Rectangle {
                anchors.left:   parent.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 1
                width:  Math.max(0, (parent.width - 2) * surveyProgressCol._progress)
                radius: parent.radius
                color:  surveyProgressCol._progress >= 1.0 ? qgcPal.colorGreen : qgcPal.colorOrange
            }
        }
    }

    LabelledLabel {
        label:     qsTr("Base Position")
        labelText: _hasFact("baseLatitude") && _hasFact("baseLongitude")
                   ? GPSFormatter.formatCoordinate(_fg.baseLatitude.value, _fg.baseLongitude.value)
                   : _valueNA
        visible:   _hasFact("baseLatitude") && !isNaN(_fg.baseLatitude.value) && _fg.baseLatitude.value !== 0
    }

    LabelledLabel {
        label:     qsTr("Heading")
        labelText: _hasFact("heading") ? GPSFormatter.formatHeading(_fg.heading.value) : _valueNA
        visible:   _factVal("headingValid") === true
    }

    LabelledLabel {
        label:     qsTr("Baseline")
        labelText: _hasFact("baselineLength") ? (_fg.baselineLength.value.toFixed(3) + " m") : _valueNA
        visible:   _factVal("baselineLength") > 0
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight * 0.25
        visible: !!_sat && (_sat.usedCount || 0) > 0
        Layout.fillWidth: true

        QGCLabel {
            text: qsTr("Signal Strength")
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Row {
            id:      signalBarRow
            spacing: 1
            Layout.fillWidth: true

            Repeater {
                model: _sat

                SignalStrengthBar {
                    width: Math.max(3, (signalBarRow.width - (_sat.count - 1)) / Math.max(1, _sat.count))
                    snr:   model.snr
                    used:  model.used
                }
            }
        }
    }
}
