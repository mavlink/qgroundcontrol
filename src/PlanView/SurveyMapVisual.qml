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
    property var _polygon
    property var _grid
    property var _entryCoordinate
    property var _exitCoordinate
    property var _dragHandles
    property var _splitHandles

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        _polygon = polygonComponent.createObject(map)
        _grid = gridComponent.createObject(map)
        _entryCoordinate = entryPointComponent.createObject(map)
        _exitCoordinate = exitPointComponent.createObject(map)
        map.addMapItem(_polygon)
        map.addMapItem(_grid)
        map.addMapItem(_entryCoordinate)
        map.addMapItem(_exitCoordinate)
    }

    function _destroyVisualElements() {
        _polygon.destroy()
        _grid.destroy()
        _entryCoordinate.destroy()
        _exitCoordinate.destroy()
    }

    function _addDragHandles() {
        if (!_dragHandles) {
            _dragHandles = dragHandlesComponent.createObject(map)
        }
        if (!_splitHandles) {
            _splitHandles = splitHandlesComponent.createObject(map)
        }
    }

    function _destroyDragHandles() {
        if (_dragHandles) {
            _dragHandles.destroy()
            _dragHandles = undefined
        }
        if (_splitHandles) {
            _splitHandles.destroy()
            _splitHandles = undefined
        }
    }

    /// Add an initial 4 sided polygon if there is none
    function _addInitialPolygon() {
        if (_missionItem.polygonPath.length < 3) {
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
            _missionItem.addPolygonCoordinate(map.toCoordinate(topLeft, false /* clipToViewPort */))
            _missionItem.addPolygonCoordinate(map.toCoordinate(topRight, false /* clipToViewPort */))
            _missionItem.addPolygonCoordinate(map.toCoordinate(bottomRight, false /* clipToViewPort */))
            _missionItem.addPolygonCoordinate(map.toCoordinate(bottomLeft, false /* clipToViewPort */))
        }
    }

    Component.onCompleted: {
        _addInitialPolygon()
        _addVisualElements()
        if (_missionItem.isCurrentItem) {
            _addDragHandles()
        }
    }

    Component.onDestruction: {
        _destroyVisualElements()
        _destroyDragHandles()
    }

    Connections {
        target: _missionItem

        onIsCurrentItemChanged: {
            if (_missionItem.isCurrentItem) {
                _addDragHandles()
            } else {
                _destroyDragHandles()
            }
        }
    }

    // Survey area polygon
    Component {
        id: polygonComponent

        MapPolygon {
            color: "green"
            opacity:    0.5
            path:       _missionItem.polygonPath
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

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              QGroundControl.zOrderMapItems + 1

            property int vertexIndex

            sourceItem: Rectangle {
                id:         dragHandle
                width:      ScreenTools.defaultFontPixelHeight * 1.5
                height:     width
                radius:     width / 2
                color:      "white"
                opacity:    .50

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.verticalCenter:     parent.verticalCenter
                    text:                       "+"
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  _missionItem.splitPolygonSegment(mapQuickItem.vertexIndex)
                }
            }
        }
    }

    Component {
        id: splitHandlesComponent

        Repeater {
            model: _missionItem.polygonPath

            delegate: Item {
                property var _splitHandle
                property var _vertices:     _missionItem.polygonPath

                function _setHandlePosition() {
                    var nextIndex = index + 1
                    if (nextIndex > _vertices.length - 1) {
                        nextIndex = 0
                    }
                    var distance = _vertices[index].distanceTo(_vertices[nextIndex])
                    var azimuth = _vertices[index].azimuthTo(_vertices[nextIndex])
                    _splitHandle.coordinate = _vertices[index].atDistanceAndAzimuth(distance / 2, azimuth)
                }

                Component.onCompleted: {
                    _splitHandle = splitHandleComponent.createObject(map)
                    _splitHandle.vertexIndex = index
                    _setHandlePosition()
                    map.addMapItem(_splitHandle)
                }

                Component.onDestruction: {
                    if (_splitHandle) {
                        _splitHandle.destroy()
                    }
                }
            }
        }
    }

    // Control which is used to drag polygon vertices
    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            id: dragArea

            property int polygonVertex

            property bool _creationComplete: false

            Component.onCompleted: _creationComplete = true

            onItemCoordinateChanged: {
                if (_creationComplete) {
                    // During component creation some bad coordinate values got through which screws up polygon draw
                    _missionItem.adjustPolygonCoordinate(polygonVertex, itemCoordinate)
                }
            }

            onClicked: _missionItem.removePolygonVertex(polygonVertex)
        }
    }

    Component {
        id: dragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              QGroundControl.zOrderMapItems + 2

            sourceItem: Rectangle {
                id:         dragHandle
                width:      ScreenTools.defaultFontPixelHeight * 1.5
                height:     width
                radius:     width / 2
                color:      "white"
                opacity:    .90
            }
        }
    }

    // Add all polygon vertex drag handles to the map
    Component {
        id: dragHandlesComponent

        Repeater {
            model: _missionItem.polygonModel

            delegate: Item {
                property var _visuals: [ ]

                Component.onCompleted: {
                    var dragHandle = dragHandleComponent.createObject(map)
                    dragHandle.coordinate = Qt.binding(function() { return object.coordinate })
                    map.addMapItem(dragHandle)
                    var dragArea = dragAreaComponent.createObject(map, { "itemIndicator": dragHandle, "itemCoordinate": object.coordinate })
                    dragArea.polygonVertex = Qt.binding(function() { return index })
                    _visuals.push(dragHandle)
                    _visuals.push(dragArea)
                }

                Component.onDestruction: {
                    for (var i=0; i<_visuals.length; i++) {
                        _visuals[i].destroy()
                    }
                    _visuals = [ ]
                }
            }
        }
    }
}

