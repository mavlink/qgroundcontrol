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
    z: QGroundControl.zOrderMapItems

    property var    map
    property var    myGeoFenceController
    property bool   interactive:            false   ///< true: user can interact with items
    property bool   planView:               false   ///< true: visuals showing in plan view
    property var    homePosition

    property var _polygonComponent
    property var _dragHandles
    property var _splitHandles
    property var _breachReturnPointComponent
    property var _mouseAreaComponent
    property var _circleComponent
    property var _mapPolygon:       myGeoFenceController.mapPolygon

    function _addVisualElements() {
        _polygonComponent = polygonComponent.createObject(map)
        map.addMapItem(_polygonComponent)
        _circleComponent = circleComponent.createObject(map)
        map.addMapItem(_circleComponent)
        _breachReturnPointComponent = breachReturnPointComponent.createObject(map)
        map.addMapItem(_breachReturnPointComponent)
        _mouseAreaComponent = mouseAreaComponent.createObject(map)
    }

    function _destroyVisualElements() {
        _polygonComponent.destroy()
        _circleComponent.destroy()
        _breachReturnPointComponent.destroy()
        _mouseAreaComponent.destroy()
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
        // Initial polygon is inset to take 2/3rds space
        var rect = map.centerViewport
        rect.x += (rect.width * 0.25) / 2
        rect.y += (rect.height * 0.25) / 2
        rect.width *= 0.75
        rect.height *= 0.75
        var topLeft = Qt.point(rect.x, rect.y)
        var topRight = Qt.point(rect.x + rect.width, rect.y)
        var bottomLeft = Qt.point(rect.x, rect.y + rect.height)
        var bottomRight = Qt.point(rect.x + rect.width, rect.y + rect.height)
        _mapPolygon.clear()
        _mapPolygon.appendVertex(map.toCoordinate(topLeft, false /* clipToViewPort */))
        _mapPolygon.appendVertex(map.toCoordinate(topRight, false /* clipToViewPort */))
        _mapPolygon.appendVertex(map.toCoordinate(bottomRight, false /* clipToViewPort */))
        _mapPolygon.appendVertex(map.toCoordinate(bottomLeft, false /* clipToViewPort */))
    }

    Component.onCompleted: {
        _addVisualElements()
        _addDragHandles()
    }

    Component.onDestruction: {
        _destroyVisualElements()
        _destroyDragHandles()
    }

    Connections {
        target: myGeoFenceController

        onAddFencePolygon: {
            _addInitialPolygon()
        }
    }


    // Mouse area to capture breach return point coordinate
    Component {
        id: mouseAreaComponent

        MouseArea {
            anchors.fill:   map
            visible:        interactive
            onClicked:      myGeoFenceController.breachReturnPoint = map.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
        }
    }

    // GeoFence polygon
    Component {
        id: polygonComponent

        MapPolygon {
            z:              QGroundControl.zOrderMapItems
            border.width:   2
            border.color:   "orange"
            color:          "transparent"
            opacity:        0.5
            path:           _mapPolygon.path
            visible:        planView || geoFenceController.polygonEnabled
        }
    }

    // GeoFence circle
    Component {
        id: circleComponent

        MapCircle {
            z:              QGroundControl.zOrderMapItems
            border.width:   2
            border.color:   "orange"
            color:          "transparent"
            center:         homePosition ? homePosition : undefined
            radius:         myGeoFenceController.circleRadius
            visible:        planView || geoFenceController.circleEnabled
        }
    }

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              QGroundControl.zOrderMapItems + 1
            visible:        interactive

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
                    onClicked:  _mapPolygon.splitPolygonSegment(mapQuickItem.vertexIndex)
                }
            }
        }
    }

    Component {
        id: splitHandlesComponent

        Repeater {
            model: _mapPolygon.path.length > 2 ? _mapPolygon.path : 0

            delegate: Item {
                property var _splitHandle
                property var _vertices:     _mapPolygon.path

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
                    _splitHandle.destroy()
                }
            }
        }
    }

    // Control which is used to drag polygon vertices
    Component {
        id: dragAreaComponent

        MissionItemIndicatorDrag {
            id:         dragArea
            visible:    interactive

            property int polygonVertex

            property bool _creationComplete: false

            Component.onCompleted: _creationComplete = true

            onItemCoordinateChanged: {
                if (_creationComplete) {
                    // During component creation some bad coordinate values got through which screws up polygon draw
                    _mapPolygon.adjustVertex(polygonVertex, itemCoordinate)
                }
            }

            onClicked: _mapPolygon.removePolygonVertex(polygonVertex)
        }
    }

    Component {
        id: dragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              QGroundControl.zOrderMapItems + 2
            visible:        interactive

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
            model: _mapPolygon.pathModel

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

    // Breach return point
    Component {
        id: breachReturnPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     myGeoFenceController.breachReturnPoint

            sourceItem: MissionItemIndexLabel {
                label: "B"
            }
        }
    }
}
