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

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property real _vehicleAltitude: _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0
    property bool _fixedWing:       _activeVehicle ? _activeVehicle.fixedWing : false
    property real _sliderMaxAlt:    _fixedWing ? _maxAlt : Math.min(_vehicleAltitude + 10, _maxAlt)
    property real _sliderMinAlt:    _fixedWing ? _minAlt : Math.max(_vehicleAltitude - 10, _minAlt)

    function reset() {
        altSlider.value = Math.min(Math.max(altSlider.minimumValue, 0), altSlider.maximumValue)
    }

    function getValue() {
        return altSlider.value
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

            property real newAltitude: QGroundControl.metersToAppSettingsDistanceUnits(_root._vehicleAltitude + altSlider.value).toFixed(1)
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
        minimumValue:       _root._sliderMinAlt - _root._vehicleAltitude
        maximumValue:       _root._sliderMaxAlt - _root._vehicleAltitude
        zeroCentered:  true
        rotation:           180

        // We want slide up to be positive values
        transform: Rotation {
            origin.x:   altSlider.width / 2
            origin.y:   altSlider.height / 2
            angle:      180
        }
    }
}
