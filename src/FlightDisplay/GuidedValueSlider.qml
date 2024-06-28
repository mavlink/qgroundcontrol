/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Vehicle
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.SettingsManager

Item {
    id:     control
    width:  indicatorCanvas.width
    clip:   true

    enum SliderType {
        Altitude,
        Takeoff,
        Speed
    }

    property real   _sliderMaxVal:          0
    property real   _sliderMinVal:          0
    property int    _sliderType:            GuidedValueSlider.SliderType.Altitude
    property string _displayText:           ""

    property var    _unitsSettings:         QGroundControl.settingsManager.unitsSettings
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property real   _indicatorCenterPos

    property int    _fullSliderRangeIndex:  2
    property var    _rgValueRanges:         [ 400, 200, 100, 50, 25, 10 ]
    property var    _rgValidMajorTickSteps: [ 10, 25, 50, 100 ]
    property int    _fullSliderValueRange:  _rgValueRanges[_fullSliderRangeIndex]
    property int    _halfSliderValueRange:  _fullSliderValueRange / 2

    property real   _majorTickWidth:        ScreenTools.largeFontPixelWidth * 3
    property real   _minorTickWidth:        _majorTickWidth / 2
    property real   _majorTickPixelHeight:  ScreenTools.largeFontPixelHeight * 2
    property real   _sliderValuePerPixel:   _majorTickValueStep / _majorTickPixelHeight
    property real   _firstTickPixelOffset:  0
    property real   _firstPixelValue:       0

    property int     _cMajorTicks:           0
    property int    _majorTickValueStep:    10
    property int    _minorTickValueStep:    _majorTickValueStep / 2

    property int    _majorTickMaxValue:     0
    property int    _majorTickMinValue:     0

    property real   _sliderHeight:          0
    property real   _sliderValue:           _firstPixelValue - ((sliderFlickable.contentY + _indicatorCenterPos) * _sliderValuePerPixel)

    property var _qgcPal: QGroundControl.globalPalette

    /// Slider values should be in converted app units.
    function setupSlider(sliderType, minValue, maxValue, currentValue, displayText) {
        _sliderType = sliderType
        _sliderMinVal = minValue
        _sliderMaxVal = maxValue
        _displayText = displayText

        // Value indicator is normally centered. For takeoff slider it is at the bottom.
        if (_sliderType === GuidedValueSlider.SliderType.Takeoff) {
            _indicatorCenterPos = height - _margins - indicatorCanvas.height / 2
        } else {
            _indicatorCenterPos = height / 2
        }

        // Calculate the full range of the slider. We have been told a min/max but that is for clamping the selected slider values.
        // We need expand that range to take into account additional values that must be displayed above/below the value indicator
        // when it is at min/max.

        // Calculate the next major tick above/below min/max
        _majorTickMaxValue = Math.ceil(_sliderMaxVal / _majorTickValueStep) * _majorTickValueStep
        _majorTickMinValue = Math.floor(_sliderMinVal / _majorTickValueStep) * _majorTickValueStep

        // Add additional major ticks above/below min/max to ensure we can display the full visual range of the slider
        var pixelsAboveIndicator = _indicatorCenterPos
        var majorTicksVisibleAbove = Math.floor(pixelsAboveIndicator / _majorTickPixelHeight)
        _majorTickMaxValue += majorTicksVisibleAbove * _majorTickValueStep
        var pixelsBelowIndicator = height - _indicatorCenterPos
        var majorTicksVisibleBelow = Math.floor(pixelsBelowIndicator / _majorTickPixelHeight)
        _majorTickMinValue -= majorTicksVisibleBelow * _majorTickValueStep

        // We always increase the ticks by one above/below so that the we draw enough of them to cover the full range of the slider
        _majorTickMaxValue += _majorTickValueStep
        _majorTickMinValue -= _majorTickValueStep

        // Now calculate the position we draw the first tick mark such that we are not allowed to flick above the max value
        _firstTickPixelOffset = _indicatorCenterPos - ((_majorTickMaxValue - _sliderMaxVal) / _sliderValuePerPixel)
        _firstPixelValue = _majorTickMaxValue + (_firstTickPixelOffset * _sliderValuePerPixel)

        // Calculate the slider height such that we can flick below the min value
        _sliderHeight = (_firstPixelValue - _sliderMinVal) / _sliderValuePerPixel + (control.height - _indicatorCenterPos)

        // Position the slider such that the indicator is pointing to the current value
        sliderFlickable.contentY = (_firstPixelValue - currentValue) / _sliderValuePerPixel - _indicatorCenterPos

        _cMajorTicks = (_majorTickMaxValue - _majorTickMinValue) / _majorTickValueStep + 1
    }

    function _clampedSliderValue(value) {
        var decimalPlaces = 0
        if (_unitsSettings.verticalDistanceUnits.rawValue === UnitsSettings.VerticalDistanceUnitsMeters) {
            decimalPlaces = 1
        }
        return value.toFixed(decimalPlaces)
    }

    function getOutputValue() {
        return _clampedSliderValue(_sliderValue)
    }

    Rectangle {
        anchors.fill:   parent
        color:          _qgcPal.window
        opacity:        0.5
    }

    ColumnLayout {
        anchors.fill:       parent
        spacing:            0

        QGCLabel {
            Layout.fillWidth:   true
            horizontalAlignment: Text.AlignHCenter
            font.pointSize:     ScreenTools.SmallFontPointSize
            text:               _displayText
        }
      
        QGCFlickable {
            id:                 sliderFlickable
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentWidth:       sliderContainer.width
            contentHeight:      sliderContainer.height
            flickDeceleration:  0.5

            Item {
                id:     sliderContainer
                width:  control.width
                height: _sliderHeight

                // Major tick marks
                Repeater {
                    model: _cMajorTicks

                    Item {
                        width:  sliderContainer.width
                        height: 1
                        y:      _majorTickPixelHeight * index + _firstTickPixelOffset

                        Rectangle {
                            width:  _majorTickWidth
                            height: 1
                            color:  _qgcPal.text
                        }

                        QGCLabel {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text:                   _majorTickMaxValue - (_majorTickValueStep * index)
                            font.pointSize:         ScreenTools.largeFontPointSize
                        }
                    }
                }

                // Minor tick marks
                Repeater {
                    model: _cMajorTicks * 2

                    Rectangle {
                        y:      _majorTickPixelHeight / 2 * index +  + _firstTickPixelOffset
                        width:  _minorTickWidth
                        height: 1
                        color:  _qgcPal.text
                    }
                }
            }
        }
    }

    // Value indicator
    Canvas {
        id:     indicatorCanvas
        y:      _indicatorCenterPos - height / 2
        width:  Math.max(minIndicatorWidth, minTickDisplayWidth)
        height: indicatorHeight
        clip:   false

        property real indicatorHeight:      valueLabel.contentHeight
        property real pointerWidth:         ScreenTools.defaultFontPixelWidth
        property real minIndicatorWidth:    pointerWidth + (_margins * 2) + valueLabel.contentWidth
        property real minTickDisplayWidth:  _majorTickWidth + ScreenTools.defaultFontPixelWidth + ScreenTools.defaultFontPixelWidth * 3

        onPaint: {
            var ctx = getContext("2d")
            ctx.strokeStyle = _qgcPal.text
            ctx.fillStyle = _qgcPal.window
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, indicatorHeight / 2)
            ctx.lineTo(pointerWidth, indicatorHeight / 4)
            ctx.lineTo(pointerWidth, 0)
            ctx.lineTo(width - 1, 0)
            ctx.lineTo(width - 1, indicatorHeight)
            ctx.lineTo(pointerWidth, indicatorHeight)
            ctx.lineTo(pointerWidth, indicatorHeight / 4 * 3)
            ctx.lineTo(0, indicatorHeight / 2)
            ctx.fill()
            ctx.stroke()
        }

        QGCLabel {
            id:                     valueLabel
            anchors.margins:        _margins
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment:    Text.AlignRight
            verticalAlignment:      Text.AlignVCenter
            text:                   _clampedSliderValue(_sliderValue) + " " + QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
            font.pointSize:         ScreenTools.largeFontPointSize
        }
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.bottom:     parent.bottom
        spacing:            0
        width:              200
    }
}
