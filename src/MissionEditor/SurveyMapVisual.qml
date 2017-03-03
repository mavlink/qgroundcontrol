/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.7
import QtQuick.Controls 2.1
import QtLocation       5.3
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

/// Survey Complex Mission Item visuals
Item {
    property var map    ///< Map control to place item in

    property var _missionItem:  object
    property var _polygon
    property var _grid
    property var _entryCoordinate
    property var _exitCoordinate

    Component.onCompleted: {
        _polygon = polygonComponent.createObject(map)
        _grid = gridComponent.createObject(map)
        _entryCoordinate = entryPointComponent.createObject(map)
        _exitCoordinate = exitPointComponent.createObject(map)
        map.addMapItem(_polygon)
        map.addMapItem(_grid)
        map.addMapItem(_entryCoordinate)
        map.addMapItem(_exitCoordinate)
    }

    Component.onDestruction: {
        _polygon.destroy()
        _grid.destroy()
        _entryCoordinate.destroy()
        _exitCoordinate.destroy()
    }

    // Survey area polygon
    Component {
        id: polygonComponent

        MapPolygon {
            color: "green"
            opacity:    0.5
            path:       _missionItem.polygonPath
        }
    }

    // Survey grid lines
    Component {
        id: gridComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       _missionItem.gridPoints
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

            sourceItem:
                MissionItemIndexLabel {
                label:      "Entry"
                checked:    _missionItem.isCurrentItem

                onClicked: setCurrentItem(_missionItem.sequenceNumber)
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

            sourceItem:
                MissionItemIndexLabel {
                label:      "Exit"
                checked:    _missionItem.isCurrentItem

                onClicked: setCurrentItem(_missionItem.sequenceNumber)
            }
        }
    }
}
