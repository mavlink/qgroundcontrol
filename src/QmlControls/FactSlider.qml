/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.15
import QtQuick.Controls         2.15
import QtQuick.Layouts          1.15

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Slider {
    id:             control
    value:          fact.value
    snapMode:       stepSize ? Slider.SnapAlways : Slider.NoSnap
    live:           true
    bottomPadding:  sliderLabel.contentHeight
    leftPadding:    0
    rightPadding:   0

    property Fact fact

    property bool _loadComplete: false

    Component.onCompleted: _loadComplete = true

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
        implicitHeight: ScreenTools.defaultFontPixelHeight + sliderLabel.contentHeight

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
            id:         sliderLabel
            text:       control.from.toFixed(fact.decimalPlaces)
            visible:    control.value !== control.from
            anchors {
                left:   parent.left
                bottom: parent.bottom
            }
        }

        QGCLabel {
            text:       control.to.toFixed(fact.decimalPlaces)
            visible:    control.value !== control.to
            anchors {
                right:  parent.right
                bottom: parent.bottom
            }
        }

        QGCLabel {
            anchors.bottom:         parent.bottom
            x:                      control.leftPadding + (control.visualPosition * (control.availableWidth - width))
            text:                   control.value.toFixed(control.fact.decimalPlaces)
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
