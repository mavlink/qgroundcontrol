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

/// GeoFence map visuals
Item {
    z: QGroundControl.zOrderMapItems

    property var    map
    property var    myGeoFenceController
    property bool   interactive:            false   ///< true: user can interact with items
    property bool   planView:               false   ///< true: visuals showing in plan view
    property var    homePosition

    property var    _breachReturnPointComponent
    property var    _mouseAreaComponent
    property var    _circleComponent
    property var    _inclusionPolygons:             myGeoFenceController.inclusionMapPolygons
    property var    _exclusionPolygons:             myGeoFenceController.exclusionMapPolygons
    property bool   _interactive:                   interactive
    property bool   _circleSupported:               myGeoFenceController.circleRadiusFact !== null
    property bool   _circleEnabled:                 myGeoFenceController.circleEnabled
    property real   _circleRadius:                  _circleSupported ? myGeoFenceController.circleRadiusFact.rawValue : 0
    property bool   _polygonSupported:              myGeoFenceController.polygonSupported
    property bool   _polygonEnabled:                myGeoFenceController.polygonEnabled

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
        _circleComponent = circleComponent.createObject(map)
        map.addMapItem(_circleComponent)
        _breachReturnPointComponent = breachReturnPointComponent.createObject(map)
        map.addMapItem(_breachReturnPointComponent)
        _mouseAreaComponent = mouseAreaComponent.createObject(map)
    }

    Component.onDestruction: {
        _circleComponent.destroy()
        _breachReturnPointComponent.destroy()
        _mouseAreaComponent.destroy()
    }

    Connections {
        target:             myGeoFenceController

        onAddInclusionPolygon: addPolygon(true)
        onAddExclusionPolygon: addPolygon(false)
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

    Instantiator {
        model: _inclusionPolygons

        onCountChanged: console.log("Instantiator", count)

        delegate : QGCMapPolygonVisuals {
            mapControl:     map
            mapPolygon:     object
            interactive:    _interactive
            borderWidth:    2
            borderColor:    "orange"
        }
    }

    Instantiator {
        model: _exclusionPolygons

        delegate : QGCMapPolygonVisuals {
            mapControl:     map
            mapPolygon:     object
            interactive:    _interactive
            borderWidth:    2
            borderColor:    "orange"
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
            center:         homePosition ? homePosition : QtPositioning.coordinate()
            radius:         _circleRadius
            visible:        _circleSupported && _circleRadius > 0 && (planView || _circleEnabled)
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
