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

/// Structure Scan Complex Mission Item visuals
Item {
    id: _root

    property var map        ///< Map control to place item in

    property var _missionItem:      object
    property var _structurePolygon: object.structurePolygon
    property var _flightPolygon:    object.flightPolygon
    property bool interactive:      parent.interactive

    signal clicked(int sequenceNumber)

    function _addVisualElements() {
        objMgr.createObjects([entryPointComponent, exitPointComponent], map, true /* parentObjectIsMap */)
    }

    Component.onCompleted: {
        _addVisualElements()
    }

    QGCDynamicObjectManager { id: objMgr }

    QGCMapPolygonVisuals {
        mapControl:         map
        mapPolygon:         _structurePolygon
        interactive:        _missionItem.isCurrentItem && _root.interactive
        borderWidth:        1
        borderColor:        "black"
        interiorColor:      "green"
        altColor:           "red"
        interiorOpacity:    0.5 * _root.opacity
    }

    QGCMapPolygonVisuals {
        mapControl:         map
        mapPolygon:         _flightPolygon
        interactive:        false
        borderWidth:        2
        borderColor:        "white"
        interiorOpacity:    _root.opacity
    }

    // Entry point
    Component {
        id: entryPointComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            z:              QGroundControl.zOrderMapItems
            coordinate:     _missionItem.coordinate
            visible:        _missionItem.exitCoordinate.isValid && _root.interactive

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
            visible:        _missionItem.exitCoordinate.isValid && _root.interactive

            sourceItem: MissionItemIndexLabel {
                index:      _missionItem.lastSequenceNumber
                label:      "Exit"
                checked:    _missionItem.isCurrentItem
                onClicked:  _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }
}
