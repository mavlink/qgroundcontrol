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

import QGroundControl.Palette
import QGroundControl.ScreenTools

Slider {
    id:             control
    implicitHeight: ScreenTools.implicitSliderHeight
    leftPadding:    0
    rightPadding:   0
    topPadding:     0
    bottomPadding:  0

    // FIXME-QT6 - This property used to be available in Control 1. Now we will need to implement the visuals ourselves
    property bool tickmarksEnabled: false

    
    property bool zeroCentered:         false   // Value indicator starts display from zero instead of min value
    property bool displayValue:         false
    property bool indicatorBarVisible:  true

    property real _implicitBarLength:   Math.round(ScreenTools.defaultFontPixelWidth * 20)
    property real _barHeight:           Math.round(ScreenTools.defaultFontPixelHeight / 3)

    background: Rectangle {
        x:              control.horizontal ? 
                            control.leftPadding : 
                            control.leftPadding + control.availableWidth / 2 - width / 2
        y:              control.horizontal ? 
                            control.topPadding + control.availableHeight / 2 - height / 2 :
                            control.topPadding
        implicitWidth:  control.horizontal ? control._implicitBarLength : control._barHeight
        implicitHeight: control.horizontal ? control._barHeight : control._implicitBarLength
        width:          control.horizontal ? control.availableWidth : implicitWidth
        height:         control.horizontal ? implicitHeight : control.availableHeight
        radius:         control._barHeight / 2
        color:          qgcPal.button
        border.width:   1
        border.color:   qgcPal.buttonText
    }

    // FIXME-QT6: Indicator portion of slider not yet supported
/*
        Item {
            id:     indicatorBar
            clip:   true
            visible: indicatorBarVisible
            x:      control.zeroCentered ? zeroCenteredIndicatorStart : 0
            width:  control.zeroCentered ? centerIndicatorWidth : control.visualPosition
            height: parent.height

            property real zeroValuePosition:            (Math.abs(control.from) / (control.to - control.from)) * parent.width
            property real zeroCenteredIndicatorStart:   Math.min(control.visualPosition, zeroValuePosition)
            property real zeroCenteredIndicatorStop:    Math.max(control.visualPosition, zeroValuePosition)
            property real centerIndicatorWidth:         zeroCenteredIndicatorStop - zeroCenteredIndicatorStart

            Rectangle {
                anchors.fill:   parent
                color:          qgcPal.colorBlue
                border.color:   Qt.darker(color, 1.2)
                radius:         height/2
            }
        }
    }*/

    handle: Rectangle {
        x:              control.horizontal ? 
                            control.leftPadding + control.visualPosition * (control.availableWidth - width) :
                            control.leftPadding + control.availableWidth / 2 - width / 2
        y:              control.horizontal ? 
                            control.topPadding + control.availableHeight / 2 - height / 2 :
                            control.topPadding + control.visualPosition * (control.availableHeight - height)
        implicitWidth:  _radius * 2
        implicitHeight: _radius * 2
        color:          qgcPal.button
        border.color:   qgcPal.buttonText
        border.width:   1
        radius:         _radius

        property real _radius: ScreenTools.isMobile ? ScreenTools.minTouchPixels / 2 : ScreenTools.defaultFontPixelHeight / 2

        Label {
            text:               control.value.toFixed( control.to <= 1 ? 1 : 0)
            visible:            control.displayValue
            anchors.centerIn:   parent
            font.family:        ScreenTools.normalFontFamily
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.buttonText
        }
    }
}
