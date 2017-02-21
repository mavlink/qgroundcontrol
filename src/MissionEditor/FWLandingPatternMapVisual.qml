/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.2
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

/// Fixed Wing Landing Pattern map visuals
Item {
    property var map    ///< Map control to place item in

    property var _loiterPoint
    property var _flightPath

    Component.onCompleted: {
        _flightPath = flightPathComponent.createObject(map)
        _loiterPoint = loiterComponent.createObject(map)
        map.addMapItem(_flightPath)
        map.addMapItem(_loiterPoint)
    }

    Component.onDestruction: {
        _loiterPoint.destroy()
        _flightPath.destroy()
    }

    // Flight path
    Component {
        id: flightPathComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       [ object.loiterCoordinate, object.exitCoordinate ]
        }
    }

    // Loiter point
    Component {
        id: loiterComponent

        MapQuickItem {
            anchorPoint.x:  sourceItem.width  / 2
            anchorPoint.y:  sourceItem.height / 2
            coordinate:     object.loiterCoordinate

            sourceItem:
                MissionItemIndexLabel {
                    label:      "L"
                }
        }
    }
}
