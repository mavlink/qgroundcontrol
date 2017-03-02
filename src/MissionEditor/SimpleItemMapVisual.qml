/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.2
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

/// Simple Mission Item visuals
Item {
    property var map    ///< Map control to place item in

    property var _missionItem:  object
    property var _itemVisual
    property var _dragArea

    function hideItemVisuals() {
        _itemVisual.destroy()
        _itemVisual = undefined
    }

    function showItemVisuals() {
        if (!_itemVisual) {
            _itemVisual = indicatorComponent.createObject(map)
            map.addMapItem(_itemVisual)
        }
    }

    function hideDragArea() {
        if (_dragArea) {
            _dragArea.destroy()
            _dragArea = undefined
        }
    }

    function showDragArea() {
        if (!_dragArea) {
            _dragArea = dragAreaComponent.createObject(map)
        }
    }

    Component.onCompleted: {
        showItemVisuals()
        if (_missionItem.isCurrentItem) {
            showDragArea()
        }
    }

    Component.onDestruction: {
        hideDragArea()
        hideItemVisuals()
    }


    Connections {
        target: _missionItem

        onIsCurrentItemChanged: {
            if (_missionItem.isCurrentItem) {
                showDragArea()
            } else {
                hideDragArea()
            }
        }
    }

    // Control which is used to drag items
    Component {
        id: dragAreaComponent

        Rectangle {
            id:             itemDragger
            x:              _itemVisual.x - _expandMargin
            y:              _itemVisual.y - _expandMargin
            width:          _itemVisual.width + (_expandMargin * 2)
            height:         _itemVisual.height + (_expandMargin * 2)
            color:          "transparent"
            z:              QGroundControl.zOrderMapItems + 1    // Above item icons

            property bool   dragLoiter
            property bool   _preventCoordinateBindingLoop:  false
            property real   _expandMargin:                  ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth : 0

            onXChanged: liveDrag()
            onYChanged: liveDrag()

            function liveDrag() {
                if (!itemDragger._preventCoordinateBindingLoop && Drag.active) {
                    var point = Qt.point(itemDragger.x + _expandMargin + _itemVisual.anchorPoint.x, itemDragger.y + _expandMargin + _itemVisual.anchorPoint.y)
                    var coordinate = map.toCoordinate(point)
                    itemDragger._preventCoordinateBindingLoop = true
                    coordinate.altitude = _missionItem.coordinate.altitude
                    _missionItem.coordinate = coordinate
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
    }

    Component {
        id: indicatorComponent

        MissionItemIndicator {
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.specifiesCoordinate
            z:              QGroundControl.zOrderMapItems
            missionItem:    _missionItem
            sequenceNumber: _missionItem.sequenceNumber

            onClicked: setCurrentItem(_missionItem.sequenceNumber)

            // These are the non-coordinate child mission items attached to this item
            Row {
                anchors.top:    parent.top
                anchors.left:   parent.right

                Repeater {
                    model: _missionItem.childItems

                    delegate: MissionItemIndexLabel {
                        label:                  object.abbreviation
                        checked:                object.isCurrentItem
                        z:                      2
                        specifiesCoordinate:    false

                        onClicked: setCurrentItem(object.sequenceNumber)
                    }
                }
            }
        }
    }
}
