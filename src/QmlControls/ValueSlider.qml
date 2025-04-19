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
import Qt.labs.animation

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Vehicle
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.SettingsManager

Control {
    id: control

    property real   value:              0
    property real   from:               0
    property real   to:                 100
    property string unitsString
    property string label

    required property int   decimalPlaces
    required property real  majorTickStepSize

    property int _tickValueDecimalPlaces: _countDecimalPlaces(majorTickStepSize)

    property real   _indicatorCenterPos:    width / 2

    property real   _majorTickSpacing:      ScreenTools.defaultFontPixelWidth * 6

    property real   _majorTickSize:         valueIndicator.pointerSize + valueIndicator.indicatorValueMargins
    property real   _tickValueEdgeMargin:   ScreenTools.defaultFontPixelWidth / 2
    property real   _minorTickSize:        _majorTickSize / 2
    property real   _sliderValuePerPixel:   majorTickStepSize / _majorTickSpacing

    property int    _minorTickValueStep:    majorTickStepSize / 2

    //property real   _sliderValue:           _firstPixelValue + ((sliderFlickable.contentX + _indicatorCenterPos) * _sliderValuePerPixel)

    // Calculate the full range of the slider. We have been given a min/max but that is for clamping the selected slider values.
    // We need expand that range to take into account additional values that must be displayed above/below the value indicator
    // when it is at min/max.

    // Add additional major ticks above/below min/max to ensure we can display the full visual range of the slider
    property int    _majorTicksVisibleBeyondIndicator:  Math.ceil(_indicatorCenterPos / _majorTickSpacing)
    property int    _majorTicksExtentsAdjustment:       _majorTicksVisibleBeyondIndicator * majorTickStepSize

    // Calculate the min/max for the full slider range
    property int    _majorTickMinValue: Math.floor((from - _majorTicksExtentsAdjustment) / majorTickStepSize) * majorTickStepSize
    property int    _majorTickMaxValue: Math.floor((to + _majorTicksExtentsAdjustment) / majorTickStepSize) * majorTickStepSize

    // Now calculate the position we draw the first tick mark such that we are not allowed to flick above the max value
    property real   _firstTickPixelOffset:  _indicatorCenterPos - ((from - _majorTickMinValue) / _sliderValuePerPixel)
    property real   _firstPixelValue:       _majorTickMinValue - (_firstTickPixelOffset * _sliderValuePerPixel)

    property int     _cMajorTicks: (_majorTickMaxValue - _majorTickMinValue) / majorTickStepSize + 1

    // Calculate the slider width such that we can flick through the full range of the slider
    property real   _sliderContentSize: ((to - _firstPixelValue) / _sliderValuePerPixel) + (background.width - _indicatorCenterPos)

    property bool    _loadComplete: false

    property var qgcPal: QGroundControl.globalPalette

    function setValue(value) {
        value = _clampedSliderValue(value)
        if (value !== control.value) {
            control.value = value
            _recalcSliderPos(false)
        }
    }    

    Component.onCompleted: {
        _recalcSliderPos(false)
        _loadComplete = true
    }

    onWidthChanged: {
        if (_loadComplete) {
            _recalcSliderPos()
        }
    }

    function _countDecimalPlaces(number) {
        const numberString = number.toString()
        if (numberString.includes('.')) {
            return numberString.split('.')[1].length
        } else {
            return 0
        }
    }

    function _sliderXPosToValue(xPos) {
        return _firstPixelValue + ((-xPos + _indicatorCenterPos) * _sliderValuePerPixel)
    }

    function _valueToSliderXPos(value) {
        return -((value - _firstPixelValue) / _sliderValuePerPixel) + _indicatorCenterPos
    }

    function _recalcSliderPos(animate = true) {
        // Position the slider such that the indicator is pointing to the current value
        let sliderXPos = _valueToSliderXPos(value)
        if (animate) {
            flickableAnimation.from = sliderContainer.x
            flickableAnimation.to = sliderXPos
            flickableAnimation.start()
        } else {
            sliderContainer.x = sliderXPos
        }
    }

    function _clampedSliderValue(value) {
        return Math.min(Math.max(value, from), to).toFixed(decimalPlaces)
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    // This TapHandler ensures that the slider captures touch and click events,
    // preventing them from passing through to the underlying map.
    TapHandler {
        acceptedButtons: Qt.AllButtons
        onTapped: control.forceActiveFocus()
        grabPermissions: PointerHandler.CanTakeOverFromAnything
    }

    background: Item {
        implicitHeight: _majorTickSize + tickValueMargin + ScreenTools.defaultFontPixelHeight + labelOffset
        clip:           true

        property real tickValueMargin:  ScreenTools.defaultFontPixelHeight / 3
        property real labelOffset:      labelItem.visible ? labelItem.contentHeight / 2 : 0

        Item {
            id:     sliderContainer
            y:      background.labelOffset
            width:  _sliderContentSize
            height: background.height - y

            onXChanged: {
                if (dragHandler.active) {
                    value = _sliderXPosToValue(x)
                }
            }

            DragHandler {
                id:             dragHandler
                yAxis.enabled:  false
            }

            BoundaryRule on x {
                minimum: _valueToSliderXPos(to)
                maximum: 0
            }

            PropertyAnimation on x {
                id:             flickableAnimation
                duration:       500
                easing.type:    Easing.OutCubic
                running:        false
            }

            // Major tick marks
            Repeater {
                model: _cMajorTicks

                Item {
                    width:      1
                    height:     sliderContainer.height
                    x:          _majorTickSpacing * index + _firstTickPixelOffset
                    opacity:    tickValue < from || tickValue > to ? 0.5 : 1

                    property real tickValue: _majorTickMinValue + (majorTickStepSize * index)

                    Rectangle {
                        id:     majorTickMark
                        width:  1
                        height: _majorTickSize
                        color:  qgcPal.text
                    }

                    QGCLabel {
                        anchors.bottomMargin:       _tickValueEdgeMargin
                        anchors.bottom:             parent.bottom
                        anchors.horizontalCenter:   majorTickMark.horizontalCenter
                        text:                       parent.tickValue.toFixed(_tickValueDecimalPlaces)
                    }
                }
            }

            // Minor tick marks
            Repeater {
                model: _cMajorTicks * 2

                Rectangle {
                    x:          _majorTickSpacing / 2 * index +  + _firstTickPixelOffset
                    width:      1
                    height:     _minorTickSize
                    color:      qgcPal.text
                    opacity:    tickValue < from || tickValue > to ? 0.5 : 1
                    visible:    index % 2 === 1

                    property real tickValue: _majorTickMaxValue - ((majorTickStepSize  / 2) * index)
                }
            }
        }

        Rectangle {
            id:         labelItemBackground
            width:      labelItem.contentWidth
            height:     labelItem.contentHeight
            color:      qgcPal.window
            opacity:    0.8
            visible:    labelItem.visible
        }

        QGCLabel {
            id:                 labelItem
            anchors.left:       labelItemBackground.left
            anchors.top:        labelItemBackground.top
            text:               label
            visible:            label !== ""
        }
    }

    contentItem: Item {
        implicitHeight: valueIndicator.height

        Canvas {
            id:                         valueIndicator
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            width:                      Math.max(valueLabel.contentWidth + (indicatorValueMargins * 2), pointerSize * 2 + 2)
            height:                     valueLabel.contentHeight + (indicatorValueMargins * 2) + pointerSize

            property real indicatorValueMargins:    ScreenTools.defaultFontPixelWidth / 2
            property real indicatorHeight:          valueLabel.contentHeight
            property real pointerSize:              ScreenTools.defaultFontPixelWidth

            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = qgcPal.text
                ctx.fillStyle = qgcPal.window
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(width / 2, 0)
                ctx.lineTo(width / 2 + pointerSize, pointerSize)
                ctx.lineTo(width - 1, pointerSize)
                ctx.lineTo(width - 1, height - 1)
                ctx.lineTo(1, height - 1)
                ctx.lineTo(1, pointerSize)
                ctx.lineTo(width / 2 - pointerSize, pointerSize)
                ctx.closePath()
                ctx.fill()
                ctx.stroke()
            }

            QGCLabel {
                id:                         valueLabel
                anchors.bottomMargin:       parent.indicatorValueMargins
                anchors.bottom:             parent.bottom
                anchors.horizontalCenter:   parent.horizontalCenter
                horizontalAlignment:        Text.AlignHCenter
                verticalAlignment:          Text.AlignBottom
                text:                       _clampedSliderValue(value) + (unitsString !== "" ? " " + unitsString : "")
            }

            QGCMouseArea {
                anchors.fill: parent
                onClicked: {
                    sliderValueTextField.text = _clampedSliderValue(value)
                    sliderValueTextField.visible = true
                    sliderValueTextField.forceActiveFocus()
                }
            }

            QGCTextField {
                id:                 sliderValueTextField
                anchors.topMargin:  valueIndicator.pointerSize
                anchors.fill:       parent
                showUnits:          true
                unitsLabel:         unitsString
                visible:            false
                numericValuesOnly:  true

                onEditingFinished: {
                    visible = false
                    focus = false
                    value = _clampedSliderValue(parseFloat(text))
                    _recalcSliderPos()
                }

                Connections {
                    target: control
                    onValueChanged: sliderValueTextField.visible = false
                }
            }
        }
    }
}
