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
    height: sRow.height
    width:  ScreenTools.defaultFontPixelWidth * 50

    property alias  text:   label.text
    property var    value:  0

    property real   _indicatorWidth: ScreenTools.defaultFontPixelWidth * 30

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Row {
        id:             sRow
        width:          _root.width
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCLabel {
            id:         label
            width:      ScreenTools.defaultFontPixelWidth * 8
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            text:       _root.value
            width:      ScreenTools.defaultFontPixelWidth * 6
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            id:         indicator
            width:      _indicatorWidth * (_root.value / 4096.0)
            height:     ScreenTools.defaultFontPixelHeight
            color:      qgcPal.colorGreen
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
