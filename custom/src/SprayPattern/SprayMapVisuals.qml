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
    property int    _transectCount:             _transectPoints.length / (_hasTurnaround ? 4 : 2)
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

    // Area polygon
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
