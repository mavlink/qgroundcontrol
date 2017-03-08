/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick      2.3
import QtLocation   5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

/// Use the drag a MissionItemIndicator
Rectangle {
    id:             itemDragger
    x:              itemIndicator.x - _touchMarginHorizontal
    y:              itemIndicator.y - _touchMarginVertical
    width:          itemIndicator.width + (_touchMarginHorizontal * 2)
    height:         itemIndicator.height + (_touchMarginVertical * 2)
    color:          "transparent"
    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

    // Properties which must be specific by consumer
    property var itemIndicator  ///< The mission item indicator to drag around
    property var itemCoordinate ///< Coordinate we are updating during drag

    property bool   _preventCoordinateBindingLoop:  false

    property bool _mobile:                  true//ScreenTools.isMobile
    property real _touchWidth:              Math.max(itemIndicator.width, ScreenTools.minTouchPixels)
    property real _touchHeight:             Math.max(itemIndicator.height, ScreenTools.minTouchPixels)
    property real _touchMarginHorizontal:   _mobile ? (_touchWidth - itemIndicator.width) / 2 : 0
    property real _touchMarginVertical:     _mobile ? (_touchHeight - itemIndicator.height) / 2 : 0

    onXChanged: liveDrag()
    onYChanged: liveDrag()

    function liveDrag() {
        if (!itemDragger._preventCoordinateBindingLoop && Drag.active) {
            var point = Qt.point(itemDragger.x + _touchMarginHorizontal + itemIndicator.anchorPoint.x, itemDragger.y + _touchMarginVertical + itemIndicator.anchorPoint.y)
            var coordinate = map.toCoordinate(point)
            itemDragger._preventCoordinateBindingLoop = true
            coordinate.altitude = itemCoordinate.altitude
            itemCoordinate = coordinate
            itemDragger._preventCoordinateBindingLoop = false
        }
    }

    Drag.active: itemDrag.drag.active

    QGCMouseArea {
        id:                 itemDrag
        anchors.fill:       parent
        drag.target:        parent
        drag.minimumX:      0
        drag.minimumY:      0
        drag.maximumX:      itemDragger.parent.width - parent.width
        drag.maximumY:      itemDragger.parent.height - parent.height
        preventStealing:    true
    }
}
