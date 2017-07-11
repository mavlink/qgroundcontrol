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

/// QGCMapPolygon map visuals
Item {
    id: _root

    property var    mapControl                          ///< Map control to place item in
    property var    mapPolygon                          ///< QGCMapPolygon object
    property bool   interactive:        mapPolygon.interactive
    property color  interiorColor:      "transparent"
    property real   interiorOpacity:    1
    property int    borderWidth:        0
    property color  borderColor:        "black"

    property var    _polygonComponent
    property var    _dragHandlesComponent
    property var    _splitHandlesComponent
    property var    _centerDragHandleComponent

    property real _zorderDragHandle:    QGroundControl.zOrderMapItems + 3   // Highest to prevent splitting when items overlap
    property real _zorderSplitHandle:   QGroundControl.zOrderMapItems + 2
    property real _zorderCenterHandle:  QGroundControl.zOrderMapItems + 1   // Lowest such that drag or split takes precedence

    function addVisuals() {
        _polygonComponent = polygonComponent.createObject(mapControl)
        mapControl.addMapItem(_polygonComponent)
    }

    function removeVisuals() {
        _polygonComponent.destroy()
    }

    function addHandles() {
        if (!_dragHandlesComponent) {
            _dragHandlesComponent = dragHandlesComponent.createObject(mapControl)
            _splitHandlesComponent = splitHandlesComponent.createObject(mapControl)
            _centerDragHandleComponent = centerDragHandleComponent.createObject(mapControl)
        }
    }

    function removeHandles() {
        if (_dragHandlesComponent) {
            _dragHandlesComponent.destroy()
            _dragHandlesComponent = undefined
        }
        if (_splitHandlesComponent) {
            _splitHandlesComponent.destroy()
            _splitHandlesComponent = undefined
        }
        if (_centerDragHandleComponent) {
            _centerDragHandleComponent.destroy()
            _centerDragHandleComponent = undefined
        }
    }

    /// Add an initial 4 sided polygon
    function addInitialPolygon() {
        if (mapPolygon.count < 3) {
            // Initial polygon is inset to take 2/3rds space
            var rect = Qt.rect(map.centerViewport.x, map.centerViewport.y, map.centerViewport.width, map.centerViewport.height)
            rect.x += (rect.width * 0.25) / 2
            rect.y += (rect.height * 0.25) / 2
            rect.width *= 0.75
            rect.height *= 0.75

            var centerCoord =       map.toCoordinate(Qt.point(rect.x + (rect.width / 2), rect.y + (rect.height / 2)),   false /* clipToViewPort */)
            var topLeftCoord =      map.toCoordinate(Qt.point(rect.x, rect.y),                                          false /* clipToViewPort */)
            var topRightCoord =     map.toCoordinate(Qt.point(rect.x + rect.width, rect.y),                             false /* clipToViewPort */)
            var bottomLeftCoord =   map.toCoordinate(Qt.point(rect.x, rect.y + rect.height),                            false /* clipToViewPort */)
            var bottomRightCoord =  map.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height),               false /* clipToViewPort */)

            // Initial polygon has max width and height of 3000 meters
            var halfWidthMeters =   Math.min(topLeftCoord.distanceTo(topRightCoord), 3000) / 2
            var halfHeightMeters =  Math.min(topLeftCoord.distanceTo(bottomLeftCoord), 3000) / 2
            topLeftCoord =      centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 0)
            topRightCoord =     centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 0)
            bottomLeftCoord =   centerCoord.atDistanceAndAzimuth(halfWidthMeters, -90).atDistanceAndAzimuth(halfHeightMeters, 180)
            bottomRightCoord =  centerCoord.atDistanceAndAzimuth(halfWidthMeters, 90).atDistanceAndAzimuth(halfHeightMeters, 180)

            mapPolygon.appendVertex(topLeftCoord)
            mapPolygon.appendVertex(topRightCoord)
            mapPolygon.appendVertex(bottomRightCoord)
            mapPolygon.appendVertex(bottomLeftCoord)
        }
    }

    onInteractiveChanged: {
        if (interactive) {
            addHandles()
        } else {
            removeHandles()
        }
    }

    Component.onCompleted: {
        addVisuals()
        if (interactive) {
            addHandles()
        }
    }

    Component.onDestruction: {
        removeVisuals()
        removeHandles()
    }

    Component {
        id: polygonComponent

        MapPolygon {
            color:          interiorColor
            opacity:        interiorOpacity
            border.color:   borderColor
            border.width:   borderWidth
            path:           mapPolygon.path
        }
    }

    Component {
        id: splitHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2

            property int vertexIndex

            sourceItem: Rectangle {
                id:         dragHandle
                width:      ScreenTools.defaultFontPixelHeight * 1.5
                height:     width
                radius:     width / 2
                color:      "white"
                opacity:    .50
                z:          _zorderSplitHandle

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.verticalCenter:     parent.verticalCenter
                    text:                       "+"
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  mapPolygon.splitPolygonSegment(mapQuickItem.vertexIndex)
                }
            }
        }
    }

    Component {
        id: splitHandlesComponent

        Repeater {
            model: mapPolygon.path

            delegate: Item {
                property var _splitHandle
                property var _vertices:     mapPolygon.path

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
                    _splitHandle = splitHandleComponent.createObject(mapControl)
                    _splitHandle.vertexIndex = index
                    _setHandlePosition()
                    mapControl.addMapItem(_splitHandle)
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
            z:  _zorderDragHandle

            property int polygonVertex

            property bool _creationComplete: false

            Component.onCompleted: _creationComplete = true

            onItemCoordinateChanged: {
                if (_creationComplete) {
                    // During component creation some bad coordinate values got through which screws up polygon draw
                    mapPolygon.adjustVertex(polygonVertex, itemCoordinate)
                }
            }

            onClicked: mapPolygon.removeVertex(polygonVertex)
        }
    }

    Component {
        id: dragHandleComponent

        MapQuickItem {
            id:             mapQuickItem
            anchorPoint.x:  dragHandle.width / 2
            anchorPoint.y:  dragHandle.height / 2
            z:              _zorderDragHandle

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
            model: mapPolygon.pathModel

            delegate: Item {
                property var _visuals: [ ]

                Component.onCompleted: {
                    var dragHandle = dragHandleComponent.createObject(mapControl)
                    dragHandle.coordinate = Qt.binding(function() { return object.coordinate })
                    mapControl.addMapItem(dragHandle)
                    var dragArea = dragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": object.coordinate })
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

    Component {
        id: centerDragAreaComponent

        MissionItemIndicatorDrag {
            z:                          _zorderCenterHandle
            onItemCoordinateChanged:    mapPolygon.center = itemCoordinate
            onDragStart:                mapPolygon.centerDrag = true
            onDragStop:                 mapPolygon.centerDrag = false
        }
    }

    Component {
        id: centerDragHandleComponent

        Item {
            property var dragHandle
            property var dragArea

            Component.onCompleted: {
                dragHandle = dragHandleComponent.createObject(mapControl)
                dragHandle.coordinate = Qt.binding(function() { return mapPolygon.center })
                mapControl.addMapItem(dragHandle)
                dragArea = centerDragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": mapPolygon.center })
            }

            Component.onDestruction: {
                dragHandle.destroy()
                dragArea.destroy()
            }
        }
    }
}

