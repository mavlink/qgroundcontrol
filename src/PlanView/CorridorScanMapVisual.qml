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
import QGroundControl.Controls      1.0

/// Corridor Scan Complex Mission Item visuals
Item {
    id: _root

    property var map        ///< Map control to place item in
    property var qgcView    ///< QGCView to use for popping dialogs

    property var _missionItem:      object
    property var _entryCoordinate
    property var _exitCoordinate
    property var _transectLines

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        _entryCoordinate = entryPointComponent.createObject(map)
        _exitCoordinate = exitPointComponent.createObject(map)
        _transectLines = transectsComponent.createObject(map)
        map.addMapItem(_entryCoordinate)
        map.addMapItem(_exitCoordinate)
        map.addMapItem(_transectLines)
    }

    function _destroyVisualElements() {
        _entryCoordinate.destroy()
        _exitCoordinate.destroy()
        _transectLines.destroy()
    }

    Component.onCompleted: {
        mapPolylineVisuals.addInitialPolyline()
        _addVisualElements()
    }

    Component.onDestruction: {
        _destroyVisualElements()
    }

    QGCMapPolygonVisuals {
        qgcView:            _root.qgcView
        mapControl:         map
        mapPolygon:         object.surveyAreaPolygon
        interactive:        false
        interiorColor:      "green"
        interiorOpacity:    0.25
    }

    QGCMapPolylineVisuals {
        id:             mapPolylineVisuals
        qgcView:        _root.qgcView
        mapControl:     map
        mapPolyline:    object.corridorPolyline
        interactive:    _missionItem.isCurrentItem
        lineWidth:      3
        lineColor:      "#be781c"
    }

    // Entry point
    Component {
        id: entryPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.coordinate.isValid

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.sequenceNumber
                label:      "Entry"
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
                label:      "Exit"
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    // Transect lines
    Component {
        id: transectsComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _missionItem.visualTransectPoints
        }
    }
}
