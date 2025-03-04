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
    property real   _indicatorCenterPos:    sliderFlickable.height / 2

    property int    _fullSliderRangeIndex:  2
    property var    _rgValueRanges:         [ 400, 200, 100, 50, 25, 10 ]
    property var    _rgValidMajorTickSteps: [ 10, 25, 50, 100 ]
    property int    _fullSliderValueRange:  _rgValueRanges[_fullSliderRangeIndex]
    property int    _halfSliderValueRange:  _fullSliderValueRange / 2

    property real   _majorTickWidth:        ScreenTools.largeFontPixelWidth * 2
    property real   _majorTickPixelHeight:  ScreenTools.largeFontPixelHeight * 2
    property real   _tickValueRightMargin:  ScreenTools.defaultFontPixelWidth / 2
    property real   _minorTickWidth:        _majorTickWidth / 2
    property real   _sliderValuePerPixel:   _majorTickValueStep / _majorTickPixelHeight

    property int    _majorTickValueStep:    10
    property int    _minorTickValueStep:    _majorTickValueStep / 2

    property real   _sliderValue:           _firstPixelValue - ((sliderFlickable.contentY + _indicatorCenterPos) * _sliderValuePerPixel)

    // Calculate the full range of the slider. We have been given a min/max but that is for clamping the selected slider values.
    // We need expand that range to take into account additional values that must be displayed above/below the value indicator
    // when it is at min/max.

    // Add additional major ticks above/below min/max to ensure we can display the full visual range of the slider
    property int    _majorTicksVisibleAboveIndicator:   Math.floor(_indicatorCenterPos / _majorTickPixelHeight)
    property int    _majorTickAdjustment:               _majorTicksVisibleAboveIndicator * _majorTickValueStep

    // Calculate the next major tick above/below min/max
    property int    _majorTickMaxValue:     Math.ceil((_sliderMaxVal + _majorTickAdjustment)/ _majorTickValueStep) * _majorTickValueStep 
    property int    _majorTickMinValue:     Math.floor((_sliderMinVal - _majorTickAdjustment)/ _majorTickValueStep) * _majorTickValueStep

    // Now calculate the position we draw the first tick mark such that we are not allowed to flick above the max value
    property real   _firstTickPixelOffset:  _indicatorCenterPos - ((_majorTickMaxValue - _sliderMaxVal) / _sliderValuePerPixel)
    property real   _firstPixelValue:       _majorTickMaxValue + (_firstTickPixelOffset * _sliderValuePerPixel)

    // Calculate the slider height such that we can flick below the min value
    property real   _sliderHeight:          (_firstPixelValue - _sliderMinVal) / _sliderValuePerPixel + (sliderFlickable.height - _indicatorCenterPos)

    property int     _cMajorTicks:          (_majorTickMaxValue - _majorTickMinValue) / _majorTickValueStep + 1

    property var _qgcPal: QGroundControl.globalPalette

    function setCurrentValue(currentValue, animate = true) {
        // Position the slider such that the indicator is pointing to the current value
        var contentY = (_firstPixelValue - currentValue) / _sliderValuePerPixel - _indicatorCenterPos
        if (animate) {
            flickableAnimation.from = sliderFlickable.contentY
            flickableAnimation.to = contentY
            flickableAnimation.start()
        } else {
            sliderFlickable.contentY = contentY
        }
    }

    /// Slider values should be in converted app units.
    function setupSlider(sliderType, minValue, maxValue, currentValue, displayText) {
        //console.log("setupSlider: sliderType: ", sliderType, " minValue: ", minValue, " maxValue: ", maxValue, " currentValue: ", currentValue, " displayText: ", displayText)
        _sliderType = sliderType
        _sliderMinVal = minValue
        _sliderMaxVal = maxValue
        _displayText = displayText
        setCurrentValue(currentValue, false)
    }

    function _clampedSliderValueString(value) {
        var decimalPlaces = 0
        if (_unitsSettings.verticalDistanceUnits.rawValue === UnitsSettings.VerticalDistanceUnitsMeters) {
            decimalPlaces = 1
        }
        return Math.min(Math.max(value  , _sliderMinVal), _sliderMaxVal).toFixed(decimalPlaces)
    }

    function getOutputValue() {
        return parseFloat(_clampedSliderValueString(_sliderValue))
    }

    DeadMouseArea {
        anchors.fill:   parent
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
            font.pointSize:     ScreenTools.smallFontPointSize
            text:               _displayText
        }
      
        QGCFlickable {
            id:                 sliderFlickable
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentWidth:       sliderContainer.width
            contentHeight:      sliderContainer.height
            flickDeceleration:  0.5
            flickableDirection: Flickable.VerticalFlick

            PropertyAnimation on contentY {
                id:             flickableAnimation
                duration:       500
                from:           fromValue
                to:             toValue
                easing.type:    Easing.OutCubic
                running:        false

                property real fromValue
                property real toValue
            }

            Item {
                id:     sliderContainer
                width:  control.width
                height: _sliderHeight

                // Major tick marks
                Repeater {
                    model: _cMajorTicks

                    Item {
                        width:      sliderContainer.width
                        height:     1
                        y:          _majorTickPixelHeight * index + _firstTickPixelOffset
                        opacity:    tickValue < _sliderMinVal || tickValue > _sliderMaxVal ? 0.5 : 1

                        property real tickValue: _majorTickMaxValue - (_majorTickValueStep * index)

                        Rectangle {
                            id:     majorTick
                            width:  _majorTickWidth
                            height: 1
                            color:  _qgcPal.text
                        }

                        QGCLabel {
                            anchors.margins:        _tickValueRightMargin
                            anchors.right:          parent.right
                            anchors.verticalCenter: majorTick.verticalCenter
                            text:                   parent.tickValue
                            font.pointSize:         ScreenTools.largeFontPointSize
                        }
                    }
                }

                // Minor tick marks
                Repeater {
                    model: _cMajorTicks * 2

                    Rectangle {
                        y:          _majorTickPixelHeight / 2 * index +  + _firstTickPixelOffset
                        width:      _minorTickWidth
                        height:     1
                        color:      _qgcPal.text
                        opacity:    tickValue < _sliderMinVal || tickValue > _sliderMaxVal ? 0.5 : 1
                        visible:    index % 2 === 1

                        property real tickValue: _majorTickMaxValue - ((_majorTickValueStep  / 2) * index)
                    }
                }
            }
        }
    }

    // Value indicator
    Canvas {
        id:     indicatorCanvas
        y:      sliderFlickable.y + _indicatorCenterPos - height / 2
        width:  Math.max(minIndicatorWidth, maxMajorTickDisplayWidth)
        height: indicatorHeight
        clip:   false

        QGCLabel {
            id:             maxDigitsTextMeasure
            text:           "-100"
            font.pointSize: ScreenTools.largeFontPointSize
            visible:        false
        }

        property real indicatorValueMargins:    ScreenTools.defaultFontPixelWidth / 2
        property real indicatorHeight:          valueLabel.contentHeight
        property real pointerWidth:             ScreenTools.defaultFontPixelWidth
        property real minIndicatorWidth:        pointerWidth + (indicatorValueMargins * 2) + valueLabel.contentWidth
        property real maxDigitsWidth:           maxDigitsTextMeasure.contentWidth
        property real intraTickDigitSpacing:    ScreenTools.defaultFontPixelWidth
        property real maxMajorTickDisplayWidth: _majorTickWidth + intraTickDigitSpacing + maxDigitsWidth + _tickValueRightMargin

        onPaint: {
            var ctx = getContext("2d")
            ctx.strokeStyle = _qgcPal.text
            ctx.fillStyle = _qgcPal.window
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, indicatorHeight / 2)
            ctx.lineTo(pointerWidth, indicatorHeight / 4)
            ctx.lineTo(pointerWidth, 1)
            ctx.lineTo(width - 1, 1)
            ctx.lineTo(width - 1, indicatorHeight - 1)
            ctx.lineTo(pointerWidth, indicatorHeight - 1)
            ctx.lineTo(pointerWidth, indicatorHeight / 4 * 3)
            ctx.closePath()
            ctx.fill()
            ctx.stroke()
        }

        QGCLabel {
            id:                     valueLabel
            anchors.margins:        indicatorCanvas.indicatorValueMargins
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment:    Text.AlignRight
            verticalAlignment:      Text.AlignVCenter
            text:                   _clampedSliderValueString(_sliderValue) + " " + unitsString
            font.pointSize:         ScreenTools.largeFontPointSize

            property var unitsString: _sliderType === GuidedValueSlider.Speed ? 
                                        QGroundControl.unitsConversion.appSettingsSpeedUnitsString : 
                                            QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
        }

        QGCMouseArea {
            anchors.fill: parent
            onClicked: {
                sliderValueTextField.text = _clampedSliderValueString(_sliderValue)
                sliderValueTextField.visible = true
                sliderValueTextField.forceActiveFocus()
            }
        }

        QGCTextField {
            id:                 sliderValueTextField
            anchors.leftMargin: indicatorCanvas.pointerWidth
            anchors.fill:       parent
            showUnits:          true
            unitsLabel:         valueLabel.unitsString
            visible:            false
            numericValuesOnly:  true

            onEditingFinished: {
                visible = false
                focus = false
                setCurrentValue(parseFloat(_clampedSliderValueString(parseFloat(text))))
            }

            Connections {
                target: control
                on_SliderValueChanged: sliderValueTextField.visible = false
            }
        }
    }

    ColumnLayout {
        id:                 mainLayout
        anchors.bottom:     parent.bottom
        spacing:            0
        width:              200
    }
}
