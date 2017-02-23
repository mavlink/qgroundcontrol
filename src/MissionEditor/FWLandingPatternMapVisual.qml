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
    property var _mouseArea
    property var _dragLoiter
    property var _dragLand
    property var _loiterPoint
    property var _landPoint
    property var _flightPath

    function hideItemVisuals() {
        if (_flightPath) {
            _flightPath.destroy()
            _flightPath = undefined
        }
        if (_loiterPoint) {
            _loiterPoint.destroy()
            _loiterPoint = undefined
        }
        if (_landPoint) {
            _landPoint.destroy()
            _landPoint = undefined
        }
    }

    function showItemVisuals() {
        if (!_flightPath) {
            _flightPath = flightPathComponent.createObject(map)
            map.addMapItem(_flightPath)
        }
        if (!_loiterPoint) {
            _loiterPoint = loiterPointComponent.createObject(map)
            map.addMapItem(_loiterPoint)
        }
        if (!_landPoint) {
            _landPoint = landPointComponent.createObject(map)
            map.addMapItem(_landPoint)
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
        console.log("hideDragAreas")
        if (_dragLoiter) {
            _dragLoiter.destroy()
            _dragLoiter = undefined
        }
        if (_dragLand) {
            _dragLand.destroy()
            _dragLand = undefined
        }
    }

    function showDragAreas() {
        console.log("showDragAreas")
        if (!_dragLoiter) {
            _dragLoiter = dragAreaComponent.createObject(map, { "dragLoiter": true })
        }
        if (!_dragLand) {
            _dragLand = dragAreaComponent.createObject(map, { "dragLoiter": false })
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
            console.log("onIsCurrentItemChanged", _missionItem.isCurrentItem)
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
            property var    mapQuickItem:                   dragLoiter ? _loiterPoint : _landPoint
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
            line.color: "white"
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
                label:      "P"
            }
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
            }
        }
    }
}
