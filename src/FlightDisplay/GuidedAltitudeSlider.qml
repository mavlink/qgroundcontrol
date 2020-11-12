/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var  _flyViewSettings:     QGroundControl.settingsManager.flyViewSettings
    property real _vehicleAltitude:     _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0
    property bool _fixedWing:           _activeVehicle ? _activeVehicle.fixedWing : false
    property real _sliderMaxAlt:        _flyViewSettings ? _flyViewSettings.guidedMaximumAltitude.rawValue : 0
    property real _sliderMinAlt:        _flyViewSettings ? _flyViewSettings.guidedMinimumAltitude.rawValue : 0
    property bool _flying:              _activeVehicle ? _activeVehicle.flying : false

    function reset() {
        altSlider.value = 0
    }

    function setToMinimumTakeoff() {
        altField.setToMinimumTakeoff()
    }

    /// Returns the user specified change in altitude from the current vehicle altitude
    function getAltitudeChangeValue() {
        return altField.newAltitudeMeters - _vehicleAltitude
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
            text:                       newAltitudeAppUnits + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString

            property real   altGainRange:           Math.max(_sliderMaxAlt - _vehicleAltitude, 0)
            property real   altLossRange:           Math.max(_vehicleAltitude - _sliderMinAlt, 0)
            property real   altExp:                 Math.pow(altSlider.value, 3)
            property real   altLossGain:            altExp * (altSlider.value > 0 ? altGainRange : altLossRange)
            property real   newAltitudeMeters:      _vehicleAltitude + altLossGain
            property string newAltitudeAppUnits:    QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(newAltitudeMeters).toFixed(1)

            function setToMinimumTakeoff() {
                altSlider.value = Math.pow(_activeVehicle.minimumTakeoffAltitude() / altGainRange, 1.0/3.0)
            }
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
        minimumValue:       _flying ? -1 : 0
        maximumValue:       1
        zeroCentered:       true
        rotation:           180

        // We want slide up to be positive values
        transform: Rotation {
            origin.x:   altSlider.width  / 2
            origin.y:   altSlider.height / 2
            angle:      180
        }
    }
}
