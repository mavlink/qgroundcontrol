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
import QGroundControl.Palette       1.0

/// Altitude slider for guided change altitude command
Rectangle {
    id:                 _root

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var  _flyViewSettings:     QGroundControl.settingsManager.flyViewSettings
    property real _vehicleAltitude:     _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0
    property real _sliderMaxVal:        _flyViewSettings ? _flyViewSettings.guidedMaximumAltitude.rawValue : 0
    property real _sliderMinVal:        _flyViewSettings ? _flyViewSettings.guidedMinimumAltitude.rawValue : 0
    property real _sliderCenterValue:   _vehicleAltitude
    property string _displayText:       ""

    // this holds the unitless value of the slider in the range [-1, 1]
    property var sliderValue : valueSlider.value

    onSliderValueChanged: {
        valueField.update(sliderValue)
    }

    on_SliderCenterValueChanged: {
       valueField.update(sliderValue)
    }

    // sets the the unitless value of the slider such that it corresponds to an output value of val
    function setValue(val) {
        if (val >= _sliderCenterValue) {
             valueSlider.value = Math.pow((val - _sliderCenterValue)  / Math.max(_sliderMaxVal - _sliderCenterValue, 0), 1.0/3.0)
        } else {
            valueSlider.value = -Math.pow((_sliderCenterValue - val) / Math.max(_sliderCenterValue - _sliderMinVal, 0), 1.0/3.0)
        }
    }

    function configureAsAltSlider() {
        _sliderMaxVal = _flyViewSettings ? _flyViewSettings.guidedMaximumAltitude.rawValue : 0
        _sliderMinVal = _flyViewSettings ? _flyViewSettings.guidedMinimumAltitude.rawValue : 0
        _sliderCenterValue = Qt.binding(function() { return _vehicleAltitude })
        setDisplayText("New Alt(rel)")
    }

    function reset() {
        valueSlider.value = 0
    }

    function setMinVal(min_val) {
        _sliderMinVal = min_val
    }

    function setMaxVal(max_val) {
        _sliderMaxVal = max_val
    }

    function setCenterValue(center) {
        _sliderCenterValue = center
    }

    // this text it displayed above the slider and indicates what quantity the slider controls
    function setDisplayText(text) {
        _displayText = text
    }

    /// Returns the user specified change in altitude from the current vehicle altitude
    function getAltitudeChangeValue() {
        return valueField.newValue - _vehicleAltitude
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
            text:                   _displayText
        }

        QGCLabel {
            id:                         valueField
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       newValueAppUnits + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString

            property real   newValue
            property string newValueAppUnits

            function update(value) {
                var   decreaseRange = Math.max(_sliderCenterValue - _sliderMinVal, 0)
                var   increaseRange = Math.max(_sliderMaxVal - _sliderCenterValue, 0)
                var   valExp = Math.pow(valueSlider.value, 3)
                var   delta = valExp * (valueSlider.value > 0 ? increaseRange : decreaseRange)
                newValue = _sliderCenterValue + delta
                newValueAppUnits = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(newValue).toFixed(1)
            }
        }
    }

    QGCSlider {
        id:                 valueSlider
        anchors.margins:    _margins
        anchors.top:        headerColumn.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right
        orientation:        Qt.Vertical
        minimumValue:       -1
        maximumValue:       1
        zeroCentered:       false
        rotation:           180

        // We want slide up to be positive values
        transform: Rotation {
            origin.x:   valueSlider.width  / 2
            origin.y:   valueSlider.height / 2
            angle:      180
        }
    }
}
