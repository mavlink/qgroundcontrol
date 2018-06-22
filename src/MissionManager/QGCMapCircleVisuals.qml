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

/// QGCMapCircle map visuals
Item {
    id: _root

    property var    mapControl                                  ///< Map control to place item in
    property var    mapCircle                                   ///< QGCMapCircle object
    property bool   interactive:        mapCircle.interactive   /// true: user can manipulate polygon
    property color  interiorColor:      "transparent"
    property real   interiorOpacity:    1
    property int    borderWidth:        2
    property color  borderColor:        "orange"

    property var _circleComponent
    property var _dragHandlesComponent

    function addVisuals() {
        if (!_circleComponent) {
            _circleComponent = circleComponent.createObject(mapControl)
            mapControl.addMapItem(_circleComponent)
        }
    }

    function removeVisuals() {
        if (_circleComponent) {
            _circleComponent.destroy()
            _circleComponent = undefined
        }
    }

    function addDragHandles() {
        if (!_dragHandlesComponent) {
            _dragHandlesComponent = dragHandlesComponent.createObject(mapControl)
        }
    }

    function removeDragHandles() {
        if (_dragHandlesComponent) {
            _dragHandlesComponent.destroy()
            _dragHandlesComponent = undefined
        }
    }

    function updateInternalComponents() {
        if (visible) {
            addVisuals()
            if (interactive) {
                addDragHandles()
            } else {
                removeDragHandles()
            }
        } else {
            removeVisuals()
            removeDragHandles()
        }
    }

    Component.onCompleted:  updateInternalComponents()
    onInteractiveChanged:   updateInternalComponents()
    onVisibleChanged:       updateInternalComponents()

    Component.onDestruction: {
        removeVisuals()
        removeDragHandles()
    }

    Component {
        id: circleComponent

        MapCircle {
            color:          interiorColor
            opacity:        interiorOpacity
            border.color:   borderColor
            border.width:   borderWidth
            center:         mapCircle.center
            radius:         mapCircle.radius.rawValue
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

    Component {
        id: centerDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: _root.mapControl

            onItemCoordinateChanged: mapCircle.center = itemCoordinate
        }
    }

    Component {
        id: radiusDragAreaComponent

        MissionItemIndicatorDrag {
            mapControl: _root.mapControl

            onItemCoordinateChanged: mapCircle.radius.rawValue = mapCircle.center.distanceTo(itemCoordinate)
        }
    }

    Component {
        id: dragHandlesComponent

        Item {
            property var centerDragHandle
            property var centerDragArea
            property var radiusDragHandle
            property var radiusDragArea
            property var radiusDragCoord:       QtPositioning.coordinate()
            property var circleCenterCoord:     mapCircle.center
            property real circleRadius:         mapCircle.radius.rawValue

            function calcRadiusDragCoord() {
                radiusDragCoord = mapCircle.center.atDistanceAndAzimuth(circleRadius, 90)
            }

            onCircleCenterCoordChanged: calcRadiusDragCoord()
            onCircleRadiusChanged:      calcRadiusDragCoord()

            Component.onCompleted: {
                calcRadiusDragCoord()
                radiusDragHandle = dragHandleComponent.createObject(mapControl)
                radiusDragHandle.coordinate = Qt.binding(function() { return radiusDragCoord })
                mapControl.addMapItem(radiusDragHandle)
                radiusDragArea = radiusDragAreaComponent.createObject(mapControl, { "itemIndicator": radiusDragHandle, "itemCoordinate": radiusDragCoord })
                centerDragHandle = dragHandleComponent.createObject(mapControl)
                centerDragHandle.coordinate = Qt.binding(function() { return circleCenterCoord })
                mapControl.addMapItem(centerDragHandle)
                centerDragArea = centerDragAreaComponent.createObject(mapControl, { "itemIndicator": centerDragHandle, "itemCoordinate": circleCenterCoord })
            }

            Component.onDestruction: {
                centerDragHandle.destroy()
                centerDragArea.destroy()
                radiusDragHandle.destroy()
                radiusDragArea.destroy()
            }
        }
    }
}

