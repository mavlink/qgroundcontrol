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

Rectangle {
    id:                 _root

    property var  _flyViewSettings:     QGroundControl.settingsManager.flyViewSettings
    property real _vehicleAltitude:     _activeVehicle ? _activeVehicle.altitudeRelative.rawValue : 0
    property bool _fixedWing:           _activeVehicle ? _activeVehicle.fixedWing : false
    property real _sliderMaxVal:        _flyViewSettings ? _flyViewSettings.guidedMaximumAltitude.rawValue : 0
    property real _sliderMinVal:        _flyViewSettings ? _flyViewSettings.guidedMinimumAltitude.rawValue : 0
    property real _sliderCenterValue:   _vehicleAltitude
    property string _displayText:       ""
    property bool _altSlider:         true

    property var sliderValue : valueSlider.value

    onSliderValueChanged: {
        valueField.updateFunction(sliderValue)
    }

    on_SliderCenterValueChanged: {
        valueField.updateFunction(sliderValue)
    }

    function setValue(val) {
        valueSlider.value = valueField.getSliderValueFromOutput(val)
        valueField.updateFunction(valueSlider.value)
    }

    function configureAsRelativeAltSliderExp() {
        _sliderMaxVal = _flyViewSettings ? _flyViewSettings.guidedMaximumAltitude.rawValue : 0
        _sliderMinVal = _flyViewSettings ? _flyViewSettings.guidedMinimumAltitude.rawValue : 0
        _sliderCenterValue = Qt.binding(function() { return _vehicleAltitude })
        _altSlider = true
        valueField.updateFunction = valueField.updateExpAroundCenterValue
        valueSlider.value = 0
        valueField.updateFunction(sliderValue)

    }

    function configureAsLinearSlider() {
        _altSlider = false
        valueField.updateFunction = valueField.updateLinear

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

    function setDisplayText(text) {
        _displayText = text
    }

    function getOutputValue() {
        if (_altSlider) {
            return valueField.newValue - _sliderCenterValue
        } else {
            return valueField.newValue
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
            text:                   _displayText
        }

        QGCLabel {
            id:                         valueField
            anchors.horizontalCenter:   parent.horizontalCenter
            text:                       newValueAppUnits + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString

            property real   newValue
            property string newValueAppUnits

            property var updateFunction : updateExpAroundCenterValue

            function updateExpAroundCenterValue(value) {
                var   decreaseRange = Math.max(_sliderCenterValue - _sliderMinVal, 0)
                var   increaseRange = Math.max(_sliderMaxVal - _sliderCenterValue, 0)
                var   valExp = Math.pow(valueSlider.value, 3)
                var   delta = valExp * (valueSlider.value > 0 ? increaseRange : decreaseRange)
                newValue = _sliderCenterValue + delta
                newValueAppUnits = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(newValue).toFixed(1)
            }

            function updateLinear(value) {
                // value is between -1 and 1
                newValue = _sliderMinVal + (value + 1) * 0.5 * (_sliderMaxVal - _sliderMinVal)
                newValueAppUnits = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(newValue).toFixed(1)
            }

            function getSliderValueFromOutputLinear(val) {
                return 2 * (val - _sliderMinVal) / (_sliderMaxVal - _sliderMinVal) - 1
            }

            function getSliderValueFromOutputExp(val) {
                if (val >= _sliderCenterValue) {
                    return Math.pow(val / Math.max(_sliderMaxVal - _sliderCenterValue, 0), 1.0/3.0)
                } else {
                    return -Math.pow(val / Math.max(_sliderCenterValue - _sliderMinVal, 0), 1.0/3.0)
                }
            }

            function getSliderValueFromOutput(output) {
                if (updateFunction == updateExpAroundCenterValue) {
                    return getSliderValueFromOutputExp(output)
                } else {
                    return getSliderValueFromOutputLinear(output)
                }
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
