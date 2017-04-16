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
    property var    _mapPolygon:                    myGeoFenceController.mapPolygon
    property bool   _interactive:                   interactive
    property bool   _circleSupported:               myGeoFenceController.circleRadiusFact !== null
    property bool   _circleEnabled:                 myGeoFenceController.circleEnabled
    property real   _circleRadius:                  _circleSupported ? myGeoFenceController.circleRadiusFact.rawValue : 0
    property bool   _polygonSupported:              myGeoFenceController.polygonSupported
    property bool   _polygonEnabled:                myGeoFenceController.polygonEnabled

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
        target:                     myGeoFenceController
        onAddInitialFencePolygon:   mapPolygonVisuals.addInitialPolygon()
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

    QGCMapPolygonVisuals {
        id:             mapPolygonVisuals
        mapControl:     map
        mapPolygon:     _mapPolygon
        interactive:    _interactive
        borderWidth:    2
        borderColor:    "orange"
        visible:        _polygonSupported && (planView || _polygonEnabled)
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
