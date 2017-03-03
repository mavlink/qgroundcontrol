/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.7
import QtQuick.Controls 1.4
import QtLocation       5.6
import QtPositioning    5.5

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

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
            _dragAreas.push(loiterDragAreaComponent.createObject(map))
            _dragAreas.push(landDragAreaComponent.createObject(map))
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

    // Control which is used to drag the loiter point
    Component {
        id: loiterDragAreaComponent

        MissionItemIndicatorDrag {
                itemIndicator:  _itemVisuals[_loiterPointIndex]
                itemCoordinate: _missionItem.loiterCoordinate

                onItemCoordinateChanged: _missionItem.loiterCoordinate = itemCoordinate
        }
    }

    // Control which is used to drag the loiter point
    Component {
        id: landDragAreaComponent

        MissionItemIndicatorDrag {
                itemIndicator:  _itemVisuals[_landPointIndex]
                itemCoordinate: _missionItem.landingCoordinate

                onItemCoordinateChanged: _missionItem.landingCoordinate = itemCoordinate
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
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.loiterCoordinate

            sourceItem:
                MissionItemIndexLabel {
                label:      "Loiter"
                checked:    _missionItem.isCurrentItem

                onClicked: setCurrentItem(_missionItem.sequenceNumber)
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
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.landingCoordinate

            sourceItem:
                MissionItemIndexLabel {
                label:      "Land"
                checked:    _missionItem.isCurrentItem

                onClicked: setCurrentItem(_missionItem.sequenceNumber)
            }
        }
    }
}
