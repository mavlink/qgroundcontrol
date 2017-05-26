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
    property int    borderWidth:        0
    property color  borderColor:        "black"

    property var _circleComponent
    property var _centerDragHandleComponent

    function addVisuals() {
        _circleComponent = circleComponent.createObject(mapControl)
        mapControl.addMapItem(_circleComponent)
    }

    function removeVisuals() {
        _circleComponent.destroy()
    }

    function addHandles() {
        if (!_centerDragHandleComponent) {
            _centerDragHandleComponent = centerDragHandleComponent.createObject(mapControl)
        }
    }

    function removeHandles() {
        if (_centerDragHandleComponent) {
            _centerDragHandleComponent.destroy()
            _centerDragHandleComponent = undefined
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
            onItemCoordinateChanged:    mapCircle.center = itemCoordinate
        }
    }

    Component {
        id: centerDragHandleComponent

        Item {
            property var dragHandle
            property var dragArea

            Component.onCompleted: {
                dragHandle = dragHandleComponent.createObject(mapControl)
                dragHandle.coordinate = Qt.binding(function() { return mapCircle.center })
                mapControl.addMapItem(dragHandle)
                dragArea = centerDragAreaComponent.createObject(mapControl, { "itemIndicator": dragHandle, "itemCoordinate": mapCircle.center })
            }

            Component.onDestruction: {
                dragHandle.destroy()
                dragArea.destroy()
            }
        }
    }
}

