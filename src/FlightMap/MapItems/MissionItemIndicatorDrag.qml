/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// Use to drag a MissionItemIndicator
Rectangle {
    id:             itemDragger
    x:              _itemIndicatorX - _touchMarginHorizontal
    y:              _itemIndicatorY - _touchMarginVertical
    width:          _itemIndicatorWidth + (_touchMarginHorizontal * 2)
    height:         _itemIndicatorHeight + (_touchMarginVertical * 2)
    color:          "transparent"
    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

    // Properties which must be specific by consumer
    property var mapControl     ///< Map control which contains this item
    property var itemIndicator  ///< The mission item indicator to drag around
    property var itemCoordinate ///< Coordinate we are updating during drag

    signal clicked
    signal dragStart
    signal dragStop

    property bool   _preventCoordinateBindingLoop:  false

    property real _itemIndicatorX:          itemIndicator ? itemIndicator.x : 0
    property real _itemIndicatorY:          itemIndicator ? itemIndicator.y : 0
    property real _itemIndicatorWidth:      itemIndicator ? itemIndicator.width : 0
    property real _itemIndicatorHeight:     itemIndicator ? itemIndicator.height : 0
    property bool _mobile:                  ScreenTools.isMobile
    property real _touchWidth:              Math.max(_itemIndicatorWidth, ScreenTools.minTouchPixels)
    property real _touchHeight:             Math.max(_itemIndicatorHeight, ScreenTools.minTouchPixels)
    property real _touchMarginHorizontal:   _mobile ? (_touchWidth - _itemIndicatorWidth) / 2 : 0
    property real _touchMarginVertical:     _mobile ? (_touchHeight - _itemIndicatorHeight) / 2 : 0
    property bool _dragStartSignalled:      false

    onXChanged: liveDrag()
    onYChanged: liveDrag()

    function liveDrag() {
        if (!itemDragger._preventCoordinateBindingLoop && itemDrag.drag.active) {
            var point = Qt.point(itemDragger.x + _touchMarginHorizontal + itemIndicator.anchorPoint.x, itemDragger.y + _touchMarginVertical + itemIndicator.anchorPoint.y)
            var coordinate = mapControl.toCoordinate(point, false /* clipToViewPort */)
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

        onClicked: {
            focus = true
            itemDragger.clicked()
        }

        property bool dragActive: drag.active
        onDragActiveChanged: {
            if (dragActive) {
                focus = true
                if (!_dragStartSignalled) {
                    _dragStartSignalled = true
                    dragStart()
                }
            } else {
                _dragStartSignalled = false
                dragStop()
            }
        }
    }
}
