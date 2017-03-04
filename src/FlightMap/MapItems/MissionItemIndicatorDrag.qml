/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick      2.7
import QtLocation   5.6

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

/// Use the drag a MissionItemIndicator
Rectangle {
    id:             itemDragger
    x:              itemIndicator.x - _expandMargin
    y:              itemIndicator.y - _expandMargin
    width:          itemIndicator.width + (_expandMargin * 2)
    height:         itemIndicator.height + (_expandMargin * 2)
    color:          "transparent"
    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

    // These are handy for debugging so left in for now
    //border.width:   1
    //border.color:   "white"

    // Properties which must be specific by consumer
    property var itemIndicator  ///< The mission item indicator to drag around
    property var itemCoordinate ///< Coordinate we are updating during drag

    property bool   _preventCoordinateBindingLoop:  false
    property real   _expandMargin:                  ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth : 0

    onXChanged: liveDrag()
    onYChanged: liveDrag()

    function liveDrag() {
        if (!itemDragger._preventCoordinateBindingLoop && Drag.active) {
            var point = Qt.point(itemDragger.x + _expandMargin + itemIndicator.anchorPoint.x, itemDragger.y + _expandMargin + itemIndicator.anchorPoint.y)
            var coordinate = map.toCoordinate(point)
            itemDragger._preventCoordinateBindingLoop = true
            coordinate.altitude = itemCoordinate.altitude
            itemCoordinate = coordinate
            itemDragger._preventCoordinateBindingLoop = false
        }
    }

    Drag.active: itemDrag.drag.active

    MouseArea {
        id:             itemDrag
        anchors.fill:   parent
        drag.target:    parent
        drag.minimumX:  0
        drag.minimumY:  0
        drag.maximumX:  itemDragger.parent.width - parent.width
        drag.maximumY:  itemDragger.parent.height - parent.height
    }
}
