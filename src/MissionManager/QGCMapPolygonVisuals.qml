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

    property var mapControl     ///< Map control to place item in
    property var mapPolygon     ///< QGCMapPolygon object

    property var _polygonComponent
    property var _dragHandlesComponent
    property var _splitHandlesComponent
    property var _centerDragHandleComponent

    function addVisuals() {
        _polygonComponent = polygonComponent.createObject(mapControl)
        mapControl.addMapItem(_polygonComponent)
    }

    function removeVisuals() {
        _polygonComponent.destroy()
    }

    function addHandles() {
        _dragHandlesComponent = dragHandlesComponent.createObject(mapControl)
        _splitHandlesComponent = splitHandlesComponent.createObject(mapControl)
        _centerDragHandleComponent = centerDragHandleComponent.createObject(mapControl)
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

    Component.onDestruction: {
        removeVisuals()
        removeHandles()
    }

    Component {
        id: polygonComponent

        MapPolygon {
            color: "green"
            opacity:    0.5
            path:       mapPolygon.path
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
            onItemCoordinateChanged: mapPolygon.center = itemCoordinate
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


