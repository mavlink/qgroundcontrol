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

/// Base control for both Survey and Corridor Scan map visuals
Item {
    id: _root

    property var map        ///< Map control to place item in

    property var    _missionItem:               object
    property var    _mapPolygon:                object.surveyAreaPolygon
    property bool   _currentItem:               object.isCurrentItem
    property var    _transectPoints:            _missionItem.visualTransectPoints
    property bool   _showPartialEntryExit:      !_currentItem && _missionItem.turnAroundDistance.rawValue !== 0 &&_transectPoints.length >= 2
    property var    _fullTransectsComponent:    null
    property var    _entryTransectsComponent:   null
    property var    _exitTransectsComponent:    null
    property var    _entryCoordinate
    property var    _exitCoordinate

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        _fullTransectsComponent =   fullTransectsComponent.createObject(map)
        _entryTransectsComponent =  entryTransectComponent.createObject(map)
        _exitTransectsComponent =   exitTransectComponent.createObject(map)
        _entryCoordinate =          entryPointComponent.createObject(map)
        _exitCoordinate =           exitPointComponent.createObject(map)

        map.addMapItem(_fullTransectsComponent)
        map.addMapItem(_entryTransectsComponent)
        map.addMapItem(_exitTransectsComponent)
        map.addMapItem(_entryCoordinate)
        map.addMapItem(_exitCoordinate)
    }

    function _destroyVisualElements() {
        _fullTransectsComponent.destroy()
        _entryTransectsComponent.destroy()
        _exitTransectsComponent.destroy()
        _entryCoordinate.destroy()
        _exitCoordinate.destroy()
    }

    Component.onCompleted: {
        _addVisualElements()
    }

    Component.onDestruction: {
        _destroyVisualElements()
    }

    // Area polygon
    QGCMapPolygonVisuals {
        id:                 mapPolygonVisuals
        mapControl:         map
        mapPolygon:         _mapPolygon
        interactive:        _missionItem.isCurrentItem
        borderWidth:        1
        borderColor:        "black"
        interiorColor:      "green"
        interiorOpacity:    0.5
    }

    // Full set of transects lines. Shown when item is selected.
    Component {
        id: fullTransectsComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _transectPoints
            visible:    _currentItem
        }
    }

    // Entry and exit transect lines only. Used when item is not selected.
    Component {
        id: entryTransectComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _showPartialEntryExit ? [ _transectPoints[0], _transectPoints[1] ] : []
            visible:    _showPartialEntryExit
        }
    }
    Component {
        id: exitTransectComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _showPartialEntryExit ? [ _transectPoints[lastPointIndex - 1], _transectPoints[lastPointIndex] ] : []
            visible:    _showPartialEntryExit

            property int lastPointIndex: _transectPoints.length - 1
        }
    }

    // Entry point
    Component {
        id: entryPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.exitCoordinate.isValid

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.sequenceNumber
                label:      qsTr("Entry")
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    // Exit point
    Component {
        id: exitPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.exitCoordinate
            visible:        _missionItem.exitCoordinate.isValid

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.lastSequenceNumber
                label:      qsTr("Exit")
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }
}
