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

import QGroundControl.FactSystem
import QGroundControl.Palette
import QGroundControl.ScreenTools

Slider {
    id:             control
    value:          fact.value
    from:           fact.min
    to:             fact.max
    stepSize:       isNaN(fact.increment) ? 0 : fact.increment
    snapMode:       stepSize ? Slider.SnapAlways : Slider.NoSnap
    live:           true
    bottomPadding:  sliderValueLabel.contentHeight
    leftPadding:    0
    rightPadding:   0

    property Fact fact
    property bool showUnits: true

    property bool _loadComplete:            false
    property real _minMaxVisibilityPadding: ScreenTools.defaultFontPixelWidth

    Component.onCompleted: _loadComplete = true

    function valuePlusUnits(valueString) {
        return valueString + (showUnits ? " " + fact.units : "")
    }

    Timer {
        id:             updateTimer
        interval:       1000
        repeat:         false
        running:        false
        onTriggered:    fact.value = control.value
    }

    onValueChanged: {
        if (_loadComplete && enabled) {
            // We don't want to spam the vehicle with parameter updates
            updateTimer.restart()
        }
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    background: Item {
        implicitWidth:  ScreenTools.defaultFontPixelWidth * 40
        implicitHeight: ScreenTools.defaultFontPixelHeight + sliderValueLabel.contentHeight

        Rectangle {
            id:     sliderBar
            x:      control.leftPadding
            y:      control.topPadding + (control.availableHeight / 2) - (height / 2)
            height: ScreenTools.defaultFontPixelHeight / 3
            width:  control.availableWidth
            radius: height / 2
            color:  qgcPal.button
        }

        QGCLabel {
            text:       control.from.toFixed(fact.decimalPlaces)
            visible:    sliderValueLabel.x > x + contentWidth + _minMaxVisibilityPadding
            anchors {
                left:   parent.left
                bottom: parent.bottom
            }
        }

        QGCLabel {
            text:       control.to.toFixed(fact.decimalPlaces)
            visible:    sliderValueLabel.x + sliderValueLabel.contentWidth < x - _minMaxVisibilityPadding
            anchors {
                right:  parent.right
                bottom: parent.bottom
            }
        }

        QGCLabel {
            id:                     sliderValueLabel
            anchors.bottom:         parent.bottom
            x:                      control.leftPadding + (control.visualPosition * (control.availableWidth - width))
            text:                   valuePlusUnits(control.value.toFixed(control.fact.decimalPlaces))
            horizontalAlignment:    Text.AlignHCenter
        }
    }

    handle: Rectangle {
        x:              control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y:              control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth:  implicitHeight
        implicitHeight: ScreenTools.defaultFontPixelHeight
        radius:         implicitHeight / 2
        color:          qgcPal.button
        border.color:   qgcPal.buttonText
    }
}
