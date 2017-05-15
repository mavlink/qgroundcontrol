/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Vehicle       1.0

/// Altitude slider for guided change altitude command
Rectangle {
    id:                 _root

    readonly property real _maxAlt: 121.92  // 400 feet
    readonly property real _minAlt: 3

    property var _guidedSettings:       QGroundControl.settingsManager.guidedSettings
    property var _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property real _vehicleAltitude:     _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0
    property bool _fixedWing:           _activeVehicle ? _activeVehicle.fixedWing : false
    property real _sliderMaxAlt:        _fixedWing ? _guidedSettings.fixedWingMaximumAltitude.value : _guidedSettings.vehicleMaximumAltitude.value
    property real _sliderMinAlt:        _fixedWing ? _guidedSettings.fixedWingMinimumAltitude.value : _guidedSettings.vehicleMinimumAltitude.value

    function reset() {
        altSlider.value = 0
    }

    function getValue() {
        return altField.newAltitude - _vehicleAltitude
    }

    function log10(value) {
        if (value === 0) {
            return 0
        } else {
            return Math.log(value) / Math.LN10
        }
    }

    Column {
        id:                 headerColumn
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        QGCLabel {
            anchors.left:           parent.left
            anchors.right:          parent.right
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            text:                   qsTr("New Alt(rel)")
        }

        QGCLabel {
            id:                         altField
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       Math.abs(newAltitude.toFixed(1)) + " " + QGroundControl.appSettingsDistanceUnitsString

            property real altGainRange: Math.max(_sliderMaxAlt - _vehicleAltitude, 0)
            property real altLossRange: Math.max(_vehicleAltitude - _sliderMinAlt, 0)
            property real altExp:       Math.pow(altSlider.value, 3)
            property real altLossGain:  altExp * (altSlider.value > 0 ? altGainRange : altLossRange)
            property real newAltitude: _vehicleAltitude + altLossGain // QGroundControl.metersToAppSettingsDistanceUnits(_root._vehicleAltitude + altSlider.value).toFixed(1)
        }
    }

    QGCSlider {
        id:                 altSlider
        anchors.margins:    _margins
        anchors.top:        headerColumn.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right
        orientation:        Qt.Vertical
        minimumValue:       -1
        maximumValue:       1
        zeroCentered:       true
        rotation:           180

        // We want slide up to be positive values
        transform: Rotation {
            origin.x:   altSlider.width / 2
            origin.y:   altSlider.height / 2
            angle:      180
        }
    }
}
