import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("GNSS Constellations")
    headingDescription: qsTr("Select which satellite systems to use. Default uses receiver settings.")
    showDividers:       true

    property var _rtkSettings:  QGroundControl.settingsManager.gcsGpsSettings
    property var _gpsMgr:       QGroundControl.gpsManager
    property int _manufacturer: _rtkSettings.baseReceiverManufacturers.rawValue
    property int _rtkDevCount:  _gpsMgr ? _gpsMgr.deviceCount : 0

    property int _displayId: GPSDeviceFlags.AllRtk

    function _updateDisplayId() {
        _displayId = GPSDeviceFlags.resolve(_gpsMgr, _manufacturer)
    }

    on_ManufacturerChanged: _updateDisplayId()
    on_RtkDevCountChanged: _updateDisplayId()
    Component.onCompleted: _updateDisplayId()

    visible: _displayId & GPSDeviceFlags.AllRtk

    readonly property int _gnssGps:     1
    readonly property int _gnssSbas:    2
    readonly property int _gnssGalileo: 4
    readonly property int _gnssBeidou:  8
    readonly property int _gnssGlonass: 16
    readonly property int _gnssFullMask: _gnssGps | _gnssSbas | _gnssGalileo | _gnssBeidou | _gnssGlonass

    property int _gnss: _rtkSettings.gnssSystems.rawValue

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("GPS")
        checked: _gnss === 0 || (_gnss & _gnssGps)
        enabled: _gnss !== 0
        onClicked: { var current = _gnss === 0 ? _gnssFullMask : _gnss; _rtkSettings.gnssSystems.rawValue = current ^ _gnssGps }
    }

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("SBAS")
        checked: _gnss === 0 || (_gnss & _gnssSbas)
        enabled: _gnss !== 0
        onClicked: { var current = _gnss === 0 ? _gnssFullMask : _gnss; _rtkSettings.gnssSystems.rawValue = current ^ _gnssSbas }
    }

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("Galileo")
        checked: _gnss === 0 || (_gnss & _gnssGalileo)
        enabled: _gnss !== 0
        onClicked: { var current = _gnss === 0 ? _gnssFullMask : _gnss; _rtkSettings.gnssSystems.rawValue = current ^ _gnssGalileo }
    }

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("BeiDou")
        checked: _gnss === 0 || (_gnss & _gnssBeidou)
        enabled: _gnss !== 0
        onClicked: { var current = _gnss === 0 ? _gnssFullMask : _gnss; _rtkSettings.gnssSystems.rawValue = current ^ _gnssBeidou }
    }

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("GLONASS")
        checked: _gnss === 0 || (_gnss & _gnssGlonass)
        enabled: _gnss !== 0
        onClicked: { var current = _gnss === 0 ? _gnssFullMask : _gnss; _rtkSettings.gnssSystems.rawValue = current ^ _gnssGlonass }
    }

    QGCButton {
        Layout.fillWidth: true
        text: _gnss === 0 ? qsTr("Customize") : qsTr("Reset to Receiver Defaults")
        onClicked: {
            if (_gnss === 0)
                _rtkSettings.gnssSystems.rawValue = 1 | 2 | 4 | 8 | 16
            else
                _rtkSettings.gnssSystems.rawValue = 0
        }
    }
}
