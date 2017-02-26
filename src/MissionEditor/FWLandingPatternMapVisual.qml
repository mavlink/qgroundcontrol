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

/// Fixed Wing Landing Pattern map visuals
Item {
    property var map    ///< Map control to place item in

    property var _missionItem:  object
    property var _itemVisuals: [ ]
    property var _mouseArea
    property var _dragAreas: [ ]

    readonly property int _flightPathIndex:     0
    readonly property int _loiterPointIndex:    1
    readonly property int _loiterRadiusIndex:   2
    readonly property int _landPointIndex:      3

    function hideItemVisuals() {
        for (var i=0; i<_itemVisuals.length; i++) {
            _itemVisuals[i].destroy()
        }
        _itemVisuals = [ ]
    }

    function showItemVisuals() {
        if (_itemVisuals.length === 0) {
            var itemVisual = flightPathComponent.createObject(map)
            map.addMapItem(itemVisual)
            _itemVisuals[_flightPathIndex] =itemVisual
            itemVisual = loiterPointComponent.createObject(map)
            map.addMapItem(itemVisual)
            _itemVisuals[_loiterPointIndex] = itemVisual
            itemVisual = loiterRadiusComponent.createObject(map)
            map.addMapItem(itemVisual)
            _itemVisuals[_loiterRadiusIndex] = itemVisual
            itemVisual = landPointComponent.createObject(map)
            map.addMapItem(itemVisual)
            _itemVisuals[_landPointIndex] = itemVisual
        }
    }

    function hideMouseArea() {
        if (_mouseArea) {
            _mouseArea.destroy()
            _mouseArea = undefined
        }
    }

    function showMouseArea() {
        if (!_mouseArea) {
            _mouseArea = mouseAreaComponent.createObject(map)
            map.addMapItem(_mouseArea)
        }
    }

    function hideDragAreas() {
        for (var i=0; i<_dragAreas.length; i++) {
            _dragAreas[i].destroy()
        }
        _dragAreas = [ ]
    }

    function showDragAreas() {
        if (_dragAreas.length === 0) {
            _dragAreas.push(dragAreaComponent.createObject(map, { "dragLoiter": true }))
            _dragAreas.push(dragAreaComponent.createObject(map, { "dragLoiter": false }))
        }
    }

    Component.onCompleted: {
        if (_missionItem.landingCoordSet) {
            showItemVisuals()
            if (_missionItem.isCurrentItem) {
                showDragAreas()
            }
        } else if (_missionItem.isCurrentItem) {
            showMouseArea()
        }
    }

    Component.onDestruction: {
        hideDragAreas()
        hideMouseArea()
        hideItemVisuals()
    }

    Connections {
        target: _missionItem

        onIsCurrentItemChanged: {
            if (_missionItem.isCurrentItem) {
                if (_missionItem.landingCoordSet) {
                    showDragAreas()
                } else {
                    showMouseArea()
                }
            } else {
                hideMouseArea()
                hideDragAreas()
            }
        }

        onLandingCoordSetChanged: {
            if (_missionItem.landingCoordSet) {
                hideMouseArea()
                showItemVisuals()
                showDragAreas()
            } else if (_missionItem.isCurrentItem) {
                hideDragAreas()
                showMouseArea()
            }
        }
    }

    // Mouse area to capture landing point coordindate
    Component {
        id:  mouseAreaComponent

        MouseArea {
            anchors.fill: map

            onClicked: {
                var coordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                _missionItem.landingCoordinate = coordinate
            }
        }
    }

    // Control which is used to drag items
    Component {
        id: dragAreaComponent

        Rectangle {
            id:             itemDragger
            x:              mapQuickItem ? (mapQuickItem.x + mapQuickItem.anchorPoint.x - (itemDragger.width / 2)) : 100
            y:              mapQuickItem ? (mapQuickItem.y + mapQuickItem.anchorPoint.y - (itemDragger.height / 2)) : 100
            width:          ScreenTools.defaultFontPixelHeight * 2
            height:         ScreenTools.defaultFontPixelHeight * 2
            color:          "transparent"
            z:              QGroundControl.zOrderMapItems + 1    // Above item icons

            property bool   dragLoiter
            property var    mapQuickItem:                   dragLoiter ? _itemVisuals[_loiterPointIndex] : _itemVisuals[_landPointIndex]
            property bool   _preventCoordinateBindingLoop:  false

            onXChanged: liveDrag()
            onYChanged: liveDrag()

            function liveDrag() {
                if (!itemDragger._preventCoordinateBindingLoop && Drag.active) {
                    var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                    var coordinate = map.toCoordinate(point)
                    itemDragger._preventCoordinateBindingLoop = true
                    if (dragLoiter) {
                        coordinate.altitude = _missionItem.loiterCoordinate.altitude
                        _missionItem.loiterCoordinate = coordinate
                    } else {
                        coordinate.altitude = _missionItem.landingCoordinate.altitude
                        _missionItem.landingCoordinate = coordinate
                    }
                    itemDragger._preventCoordinateBindingLoop = false
                }
            }

            Drag.active:    itemDrag.drag.active
            Drag.hotSpot.x: width  / 2
            Drag.hotSpot.y: height / 2

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

    // Flight path
    Component {
        id: flightPathComponent

        MapPolyline {
            z:          QGroundControl.zOrderMapItems - 1   // Under item indicators
            line.color: "#be781c"
            line.width: 2
            path:       _missionItem.landingCoordSet ? [ _missionItem.loiterCoordinate, _missionItem.landingCoordinate ] : undefined
        }
    }

    // Loiter point
    Component {
        id: loiterPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width  / 2
            anchorPoint.y:  sourceItem.height / 2
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.loiterCoordinate

            sourceItem:
                MissionItemIndexLabel {
                label:      "L"
            }
        }
    }

    // Loiter radius visual
    Component {
        id: loiterRadiusComponent

        MapCircle {
            z:              QGroundControl.zOrderMapItems
            center:         _missionItem.loiterCoordinate
            radius:         _missionItem.loiterRadius.value
            border.width:   2
            border.color:   "green"
            color:          "transparent"
        }
    }

    // Land point
    Component {
        id: landPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width  / 2
            anchorPoint.y:  sourceItem.height / 2
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.landingCoordinate

            sourceItem:
                MissionItemIndexLabel {
                label:      "L"
                checked:    _missionItem.isCurrentItem
            }
        }
    }
}
