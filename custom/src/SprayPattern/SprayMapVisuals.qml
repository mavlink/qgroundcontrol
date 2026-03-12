import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FlightMap


Item {
    id: _root

    property var    map                                                 ///< Map control to place item in
    property bool   polygonInteractive: true
    property bool   interactive: true
    property var    vehicle

    property var    _missionItem:               object
    property var    _mapPolygon:                object.sprayAreaPolygon
    property bool   _currentItem:               object.isCurrentItem
    property var    _transectPoints:            _missionItem.visualTransectPoints
    property int    _transectCount:             _transectPoints.length / 2
    property var    _fullTransectsComponent:    null

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        var toAdd = [ fullTransectsComponent ]
        objMgr.createObjects(toAdd, map, true /* parentObjectIsMap */)
    }

    function _destroyVisualElements() {
        objMgr.destroyObjects()
    }

    Component.onCompleted: {
        _addVisualElements()
    }

    Component.onDestruction: {
        _destroyVisualElements()
    }

    QGCDynamicObjectManager {
        id: objMgr
    }

    // Area polygon (field)
    QGCMapPolygonVisuals {
        id:                 mapPolygonVisuals
        mapControl:         map
        mapPolygon:         _mapPolygon
        interactive:        polygonInteractive && _missionItem.isCurrentItem && _root.interactive
        borderWidth:        1
        borderColor:        "black"
        interiorColor:      QGroundControl.globalPalette.surveyPolygonInterior
        altColor:           QGroundControl.globalPalette.surveyPolygonTerrainCollision
        interiorOpacity:    0.5 * _root.opacity
    }

    // Obstacle buffer zones (expanded no-fly margin) – drawn under obstacles
    Repeater {
        model: _missionItem.obstacleBufferPolygons || []
        delegate: MapPolygon {
            path:           modelData
            border.width:   1
            border.color:   "orangered"
            color:          "#44ff8800"
            opacity:        0.6 * _root.opacity
        }
    }

    // Obstacle (no-fly) polygons
    Repeater {
        model: _missionItem.obstaclePolygons ? _missionItem.obstaclePolygons.count : 0
        delegate: QGCMapPolygonVisuals {
            mapControl:     map
            mapPolygon:     _missionItem.obstaclePolygons.get(index)
            interactive:    polygonInteractive && _missionItem.isCurrentItem && _root.interactive
            borderWidth:    1
            borderColor:    "darkred"
            interiorColor:  "#88cc0000"
            interiorOpacity: 0.7 * _root.opacity
        }
    }

    // Full set of transects lines. Shown when item is selected.
    Component {
        id: fullTransectsComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _transectPoints
            visible:    _currentItem
            opacity:    _root.opacity
        }
    }
}
