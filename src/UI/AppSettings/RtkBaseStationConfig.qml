import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.RTK

/// Behaviour of the connected RTK base station. Connect/disconnect is in
/// GCS GPS Receivers; this section is JSON-gated on rtkConnected.
ColumnLayout {
    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var  _rtkSettings:  QGroundControl.settingsManager.gcsGpsSettings
    property QtObject _gpsRtk:   QGroundControl.gpsRtk
    property var  _gpsMgr:       QGroundControl.gpsManager
    property var  _useFixed:     _rtkSettings.useFixedBasePosition.rawValue
    property int  _manufacturer: _rtkSettings.baseReceiverManufacturers.rawValue
    property int  _rtkDevCount:  _gpsMgr ? _gpsMgr.deviceCount : 0
    property int  _displayId:    GPSDeviceFlags.All

    function _updateDisplayId() {
        _displayId = GPSDeviceFlags.resolve(_gpsMgr, _manufacturer)
    }
    on_ManufacturerChanged: _updateDisplayId()
    on_RtkDevCountChanged:  _updateDisplayId()
    Component.onCompleted:  _updateDisplayId()

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Output")
        showDividers:       true
        // Receiver-family branch; section visibility is gated by JSON showWhen.
        visible:            (_displayId & GPSDeviceFlags.AllRtk) !== 0

        LabelledFactComboBox {
            label: qsTr("Output Mode")
            fact:  _rtkSettings.gpsOutputMode
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Base Position Mode")
        headingDescription: qsTr("How the base station determines its precise location")
        showDividers:       true
        visible:            (_displayId & GPSDeviceFlags.AllRtk) !== 0

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 2

            QGCRadioButton {
                text:    qsTr("Survey-In")
                checked: _useFixed == 0
                onClicked: _rtkSettings.useFixedBasePosition.rawValue = 0
            }

            QGCRadioButton {
                text:    qsTr("Fixed Position")
                checked: _useFixed == 1
                onClicked: _rtkSettings.useFixedBasePosition.rawValue = 1
            }
        }

        FactSlider {
            Layout.fillWidth:      true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
            label: qsTr("Accuracy")
            fact:  _rtkSettings.surveyInAccuracyLimit
            majorTickStepSize: 0.1
            visible: _useFixed == 0
                     && _rtkSettings.surveyInAccuracyLimit.userVisible
                     && (_displayId & GPSDeviceFlags.Ublox)
        }

        FactSlider {
            Layout.fillWidth:      true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
            label: qsTr("Min Duration")
            fact:  _rtkSettings.surveyInMinObservationDuration
            majorTickStepSize: 10
            visible: _useFixed == 0
                     && _rtkSettings.surveyInMinObservationDuration.userVisible
                     && (_displayId & (GPSDeviceFlags.Ublox | GPSDeviceFlags.Femtomes | GPSDeviceFlags.Trimble))
        }

        LabelledFactTextField {
            label:   _rtkSettings.fixedBasePositionLatitude.shortDescription
            fact:    _rtkSettings.fixedBasePositionLatitude
            visible: _useFixed == 1
        }

        LabelledFactTextField {
            label:   _rtkSettings.fixedBasePositionLongitude.shortDescription
            fact:    _rtkSettings.fixedBasePositionLongitude
            visible: _useFixed == 1
        }

        LabelledFactTextField {
            label:   _rtkSettings.fixedBasePositionAltitude.shortDescription
            fact:    _rtkSettings.fixedBasePositionAltitude
            visible: _useFixed == 1
        }

        LabelledFactTextField {
            label:   _rtkSettings.fixedBasePositionAccuracy.shortDescription
            fact:    _rtkSettings.fixedBasePositionAccuracy
            visible: _useFixed == 1 && (_displayId & GPSDeviceFlags.Ublox)
        }

        LabelledButton {
            label:      qsTr("Current Base Position")
            buttonText: enabled ? qsTr("Save") : qsTr("Not Yet Valid")
            visible:    _useFixed == 1
            enabled:    _gpsRtk && _gpsRtk.valid && _gpsRtk.valid.value

            onClicked: {
                if (!_gpsRtk) return
                if (!_gpsRtk.currentLatitude || isNaN(_gpsRtk.currentLatitude.rawValue)) return
                _rtkSettings.fixedBasePositionLatitude.rawValue  = _gpsRtk.currentLatitude.rawValue
                _rtkSettings.fixedBasePositionLongitude.rawValue = _gpsRtk.currentLongitude.rawValue
                _rtkSettings.fixedBasePositionAltitude.rawValue  = _gpsRtk.currentAltitude.rawValue
                _rtkSettings.fixedBasePositionAccuracy.rawValue  = _gpsRtk.currentAccuracy.rawValue
            }
        }

        LabelledFactTextField {
            label:   _rtkSettings.headingOffset.shortDescription
            fact:    _rtkSettings.headingOffset
            visible: _displayId & (GPSDeviceFlags.Septentrio | GPSDeviceFlags.Ublox | GPSDeviceFlags.Nmea)
        }
    }
}
