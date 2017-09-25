/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Slider
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                      2.4
import QtQuick.Controls             1.4
import QtQuick.Controls.Styles      1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

Slider {
    id: _root
    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    style: SliderStyle {
        groove: Rectangle {
            implicitWidth:  _root.width
            implicitHeight: 4
            color:          qgcPal.colorBlue
            radius:         4
        }
        handle: Rectangle {
            anchors.centerIn: parent
            color:          qgcPal.button
            border.color:   qgcPal.buttonText
            border.width:   1
            implicitWidth:  _root.sliderTouchArea
            implicitHeight: _root.sliderTouchArea
            radius:         _root.sliderTouchArea * 0.5
            Label {
                text:               _root.value
                anchors.centerIn:   parent
                font.family:        ScreenTools.normalFontFamily
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.buttonText
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: {
            wheel.accepted = true
        }
    }
    readonly property real sliderTouchArea: ScreenTools.defaultFontPixelWidth * (ScreenTools.isTinyScreen ? 5 : (ScreenTools.isMobile ? 6 : 3))
}
