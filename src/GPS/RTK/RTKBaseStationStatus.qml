import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    required property var device

    property var _fg:  device ? device.factGroup : null
    property var _sat: device ? device.satelliteModel : null
    property var _rtkSettings: QGroundControl.settingsManager.rtkSettings
    property string _na:      qsTr("N/A")
    property string _valueNA: qsTr("-.--")

    heading:      device ? qsTr("RTK Base: %1").arg(device.deviceType) : qsTr("RTK Base Station")
    showDividers: true
    visible:      device ? (device.connected === true) : false

    function _formatAccuracy(rawVal) {
        if (isNaN(rawVal)) return _valueNA
        if (rawVal < 1.0) return (rawVal * 100).toFixed(1) + " cm"
        return rawVal.toFixed(3) + " m"
    }

    function _formatDuration(secs) {
        if (secs < 60) return secs + "s"
        if (secs < 3600) return Math.floor(secs / 60) + "m " + (secs % 60) + "s"
        return Math.floor(secs / 3600) + "h " + Math.floor((secs % 3600) / 60) + "m"
    }

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        Rectangle {
            width:  ScreenTools.defaultFontPixelHeight * 0.6
            height: width
            radius: width / 2
            color:  _fg && _fg.valid && _fg.valid.value ? qgcPal.colorGreen
                    : _fg && _fg.active && _fg.active.value ? qgcPal.colorOrange
                    : qgcPal.colorGrey
            Layout.alignment: Qt.AlignVCenter
        }

        QGCLabel {
            text: {
                if (_fg && _fg.valid && _fg.valid.value) return qsTr("RTK Streaming")
                if (_fg && _fg.active && _fg.active.value) return qsTr("Survey-in Active")
                return qsTr("Connecting")
            }
        }
    }

    LabelledLabel {
        label:     qsTr("Device")
        labelText: device && device.devicePath !== undefined ? device.devicePath : _na
    }

    LabelledLabel {
        label:     qsTr("Fix Type")
        labelText: _fg && _fg.baseFixType ? (_fg.baseFixType.enumStringValue || _na) : _na
        visible:   !!_fg && _fg.baseFixType !== undefined && _fg.baseFixType.rawValue > 0
    }

    LabelledLabel {
        label:     qsTr("Satellites")
        labelText: _fg && _fg.numSatellites ? _fg.numSatellites.valueString : _na
    }

    LabelledLabel {
        label:     qsTr("Constellations")
        labelText: _sat ? _sat.constellationSummary : ""
        visible:   !!_sat && _sat.constellationSummary !== undefined && _sat.constellationSummary !== ""
    }

    LabelledLabel {
        label:     qsTr("Duration")
        labelText: _fg && _fg.currentDuration ? _formatDuration(_fg.currentDuration.value) : _na
        visible:   !!_fg && _fg.currentDuration !== undefined && _fg.currentDuration.value > 0
    }

    LabelledLabel {
        label:     _fg && _fg.valid && _fg.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
        labelText: _fg && _fg.currentAccuracy ? _formatAccuracy(_fg.currentAccuracy.value) : _valueNA
        visible:   !!_fg && _fg.currentAccuracy !== undefined && _fg.currentAccuracy.value > 0
    }

    ColumnLayout {
        id:      surveyProgressCol
        spacing: 2
        visible: !!_fg && _fg.active !== undefined && _fg.active.value
                 && _fg.valid !== undefined && !_fg.valid.value
                 && _fg.currentAccuracy !== undefined && _fg.currentAccuracy.value > 0
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
        labelText: _fg && _fg.baseLatitude && _fg.baseLongitude
                   ? (_fg.baseLatitude.value.toFixed(7) + ", " + _fg.baseLongitude.value.toFixed(7))
                   : _valueNA
        visible:   !!_fg && _fg.baseLatitude !== undefined
                   && !isNaN(_fg.baseLatitude.value) && _fg.baseLatitude.value !== 0
    }

    LabelledLabel {
        label:     qsTr("Heading")
        labelText: _fg && _fg.heading ? (_fg.heading.value.toFixed(1) + "\u00B0") : _valueNA
        visible:   !!_fg && _fg.headingValid !== undefined && _fg.headingValid.value === true
    }

    LabelledLabel {
        label:     qsTr("Baseline")
        labelText: _fg && _fg.baselineLength ? (_fg.baselineLength.value.toFixed(3) + " m") : _valueNA
        visible:   !!_fg && _fg.baselineLength !== undefined && _fg.baselineLength.value > 0
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight * 0.25
        visible: !!_sat && _sat.usedCount !== undefined && _sat.usedCount > 0
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

                Rectangle {
                    width:  Math.max(3, (signalBarRow.width - (_sat.count - 1)) / Math.max(1, _sat.count))
                    height: ScreenTools.defaultFontPixelHeight * 2
                    color:  "transparent"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width:  parent.width - 1
                        height: Math.max(2, parent.height * (model.snr / 50.0))
                        color:  {
                            if (!model.used) return qgcPal.colorGrey
                            if (model.snr >= 35) return qgcPal.colorGreen
                            if (model.snr >= 20) return qgcPal.colorOrange
                            return qgcPal.colorRed
                        }
                        radius: 1
                    }
                }
            }
        }
    }
}
