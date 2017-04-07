/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightMap     1.0

/// Survey Complex Mission Item visuals
Item {
    id: _root

    property var map    ///< Map control to place item in

    property var _missionItem:      object
    property var _mapPolygon:       object.mapPolygon
    property var _gridComponent
    property var _entryCoordinate
    property var _exitCoordinate

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        _gridComponent = gridComponent.createObject(map)
        _entryCoordinate = entryPointComponent.createObject(map)
        _exitCoordinate = exitPointComponent.createObject(map)
        map.addMapItem(_gridComponent)
        map.addMapItem(_entryCoordinate)
        map.addMapItem(_exitCoordinate)
    }

    function _destroyVisualElements() {
        _gridComponent.destroy()
        _entryCoordinate.destroy()
        _exitCoordinate.destroy()
    }

    /// Add an initial 4 sided polygon if there is none
    function _addInitialPolygon() {
        if (_mapPolygon.count < 3) {
            // Initial polygon is inset to take 2/3rds space
            var rect = Qt.rect(map.centerViewport.x, map.centerViewport.y, map.centerViewport.width, map.centerViewport.height)
            console.log(map.centerViewport)
            rect.x += (rect.width * 0.25) / 2
            rect.y += (rect.height * 0.25) / 2
            rect.width *= 0.75
            rect.height *= 0.75
            console.log(map.centerViewport)
            var topLeft = Qt.point(rect.x, rect.y)
            var topRight = Qt.point(rect.x + rect.width, rect.y)
            var bottomLeft = Qt.point(rect.x, rect.y + rect.height)
            var bottomRight = Qt.point(rect.x + rect.width, rect.y + rect.height)
            _mapPolygon.appendVertex(map.toCoordinate(topLeft, false /* clipToViewPort */))
            _mapPolygon.appendVertex(map.toCoordinate(topRight, false /* clipToViewPort */))
            _mapPolygon.appendVertex(map.toCoordinate(bottomRight, false /* clipToViewPort */))
            _mapPolygon.appendVertex(map.toCoordinate(bottomLeft, false /* clipToViewPort */))
        }
    }

    Component.onCompleted: {
        _addInitialPolygon()
        _addVisualElements()
    }

    Component.onDestruction: {
        _destroyVisualElements()
    }

    QGCMapPolygonVisuals {
        id:         mapPolygonVisuals
        mapControl: map
        mapPolygon: _mapPolygon

        Component.onCompleted: {
            mapPolygonVisuals.addVisuals()
            if (_missionItem.isCurrentItem) {
                mapPolygonVisuals.addHandles()
            }
        }

        Connections {
            target: _missionItem

            onIsCurrentItemChanged: {
                if (_missionItem.isCurrentItem) {
                    mapPolygonVisuals.addHandles()
                } else {
                    mapPolygonVisuals.removeHandles()
                }
            }
        }
    }

    // Survey grid lines
    Component {
        id: gridComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _missionItem.gridPoints
        }
    }

    // Entry point
    Component {
        id: entryPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.exitCoordinate.isValid

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.sequenceNumber
                label:      "Entry"
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    // Exit point
    Component {
        id: exitPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.exitCoordinate
            visible:        _missionItem.exitCoordinate.isValid

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.lastSequenceNumber
                label:      "Exit"
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }
}
