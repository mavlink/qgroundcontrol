/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtLocation       5.3
import QtQuick.Controls 1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Vehicle       1.0

/// Marker for displaying a camera trigger on the map
MapQuickItem {
    anchorPoint.x:  sourceItem.width / 2
    anchorPoint.y:  sourceItem.height / 2

    sourceItem: Rectangle {
        width:      _radius * 2
        height:     _radius * 2
        radius:     _radius
        color:      "black"
        opacity:    0.4

        readonly property real _radius: ScreenTools.defaultFontPixelHeight * 0.6

        QGCColoredImage {
            anchors.margins:    3
            anchors.fill:       parent
            color:              "white"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            source:             "/qmlimages/camera.svg"
        }
    }
}
