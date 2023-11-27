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
import QtQuick.Controls
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

Slider {
    id:             control
    implicitHeight: ScreenTools.implicitSliderHeight

    // Value indicator starts display from zero instead of min value
    property bool zeroCentered: false
    property bool displayValue: false
    property bool indicatorBarVisible: true

    background: Item {
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth:          Math.round(ScreenTools.defaultFontPixelHeight * 4.5)
        implicitHeight:         Math.round(ScreenTools.defaultFontPixelHeight * 0.3)

        Rectangle {
            radius:         height / 2
            anchors.fill:   parent
            color:          qgcPal.button
            border.width:   1
            border.color:   qgcPal.buttonText
        }

        Item {
            id:     indicatorBar
            clip:   true
            visible: indicatorBarVisible
            x:      control.zeroCentered ? zeroCenteredIndicatorStart : 0
            width:  control.zeroCentered ? centerIndicatorWidth : control.visualPosition
            height: parent.height

            property real zeroValuePosition:            (Math.abs(control.minimumValue) / (control.maximumValue - control.minimumValue)) * parent.width
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
    }

    handle: Rectangle {
        anchors.centerIn: parent
        color:          qgcPal.button
        border.color:   qgcPal.buttonText
        border.width:   1
        implicitWidth:  _radius * 2
        implicitHeight: _radius * 2
        radius:         _radius

        property real _radius: Math.round(control.implicitHeight / 2)

        Label {
            text:               control.value.toFixed( control.maximumValue <= 1 ? 1 : 0)
            visible:            control.displayValue
            anchors.centerIn:   parent
            font.family:        ScreenTools.normalFontFamily
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.buttonText
        }
    }
}
