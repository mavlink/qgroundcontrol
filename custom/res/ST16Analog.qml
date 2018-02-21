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
 *   @brief On/Off Switch
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

Item {
    id:     _root
    height: ScreenTools.defaultFontPixelHeight
    width:  ScreenTools.defaultFontPixelWidth  * 80

    property alias  text:   label.text
    property real   value:  0

    property real   _indicatorWidth: ScreenTools.defaultFontPixelWidth * 60

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCLabel {
        id:         label
        width:      ScreenTools.defaultFontPixelWidth * 8
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
    }
    QGCLabel {
        id:         valueLabel
        text:       _root.value
        width:      ScreenTools.defaultFontPixelWidth * 6
        anchors.left: label.right
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        id:         indicator
        width:      _indicatorWidth * (_root.value / 4096.0)
        height:     ScreenTools.defaultFontPixelHeight
        color:      qgcPal.colorGreen
        anchors.left: valueLabel.right
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        id:     rect1
        width:  _indicatorWidth * 0.5
        height: indicator.height
        color:  Qt.rgba(0,0,0,0)
        anchors.left: valueLabel.right
        border.color: qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        width:  _indicatorWidth * 0.5
        height: indicator.height
        color:  Qt.rgba(0,0,0,0)
        anchors.left: rect1.right
        border.color: qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
    }
}
