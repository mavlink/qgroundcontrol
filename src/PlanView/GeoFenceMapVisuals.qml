/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// GeoFence map visuals
Item {
    id: _root
    z: QGroundControl.zOrderMapItems

    property var    map
    property var    myGeoFenceController
    property bool   interactive:            false   ///< true: user can interact with items
    property bool   planView:               false   ///< true: visuals showing in plan view
    property var    homePosition

    property var    _breachReturnPointComponent
    property var    _breachReturnDragComponent
    property var    _paramCircleFenceComponent
    property var    _polygons:                  myGeoFenceController.polygons
    property var    _circles:                   myGeoFenceController.circles
    property color  _borderColor:               "orange"
    property int    _borderWidthInclusion:      2
    property int    _borderWidthExclusion:      0
    property color  _interiorColorExclusion:    "orange"
    property color  _interiorColorInclusion:    "transparent"
    property real   _interiorOpacityExclusion:  0.2 * opacity
    property real   _interiorOpacityInclusion:  1 * opacity

    function addPolygon(inclusionPolygon) {
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

        console.log(map.center)
        console.log(topLeftCoord)
        console.log(bottomRightCoord)

        if (inclusionPolygon) {
            myGeoFenceController.addInclusion(topLeftCoord, bottomRightCoord)
        } else {
            myGeoFenceController.addExclusion(topLeftCoord, bottomRightCoord)
        }
    }

    Component.onCompleted: {
        _breachReturnPointComponent = breachReturnPointComponent.createObject(map)
        map.addMapItem(_breachReturnPointComponent)
        _breachReturnDragComponent = breachReturnDragComponent.createObject(map, { "itemIndicator": _breachReturnPointComponent })
        _paramCircleFenceComponent = paramCircleFenceComponent.createObject(map)
        map.addMapItem(_paramCircleFenceComponent)
    }

    Component.onDestruction: {
        _breachReturnPointComponent.destroy()
        _breachReturnDragComponent.destroy()
        _paramCircleFenceComponent.destroy()
    }

    // By default the parent for Instantiator.delegate item is the Instatiator itself. By there is a bug
    // in Qt which will cause a crash if this delete item has Menu item within it. Since the Menu item
    // doesn't like having a non-visual item as parent. This is likely related to hybrid QQuickWidtget+QML
    // Hence Qt folks are going to care. In order to workaround you have to parent the item to _root Item instead.
    Instantiator {
        model: _polygons

        delegate : QGCMapPolygonVisuals {
            parent:             _root
            mapControl:         map
            mapPolygon:         object
            borderWidth:        object.inclusion ? _borderWidthInclusion : _borderWidthExclusion
            borderColor:        _borderColor
            interiorColor:      object.inclusion ? _interiorColorInclusion : _interiorColorExclusion
            interiorOpacity:    object.inclusion ? _interiorOpacityInclusion : _interiorOpacityExclusion
            interactive:        _root.interactive && mapPolygon && mapPolygon.interactive
        }
    }

    Instantiator {
        model: _circles

        delegate : QGCMapCircleVisuals {
            parent:             _root
            mapControl:         map
            mapCircle:          object
            borderWidth:        object.inclusion ? _borderWidthInclusion : _borderWidthExclusion
            borderColor:        _borderColor
            interiorColor:      object.inclusion ? _interiorColorInclusion : _interiorColorExclusion
            interiorOpacity:    object.inclusion ? _interiorOpacityInclusion : _interiorOpacityExclusion
            interactive:         _root.interactive && mapCircle && mapCircle.interactive
        }
    }

    // Circular geofence specified from parameter
    Component {
        id: paramCircleFenceComponent

        MapCircle {
            color:          _interiorColorInclusion
            opacity:        _interiorOpacityInclusion
            border.color:   _borderColor
            border.width:   _borderWidthInclusion
            center:         homePosition
            radius:         _radius
            visible:        homePosition.isValid && _radius > 0

            property real _radius: myGeoFenceController.paramCircularFence

            on_RadiusChanged: console.log("_radius", _radius, homePosition.isValid, homePosition)
        }
    }

    Component {
        id: breachReturnDragComponent

        MissionItemIndicatorDrag {
            mapControl:     map
            itemCoordinate: myGeoFenceController.breachReturnPoint
            visible:        _root.interactive

            onItemCoordinateChanged: myGeoFenceController.breachReturnPoint = itemCoordinate
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
            opacity:        _root.opacity

            sourceItem: MissionItemIndexLabel {
                label:      qsTr("B", "Breach Return Point item indicator")
                checked:    true
            }
        }
    }
}
