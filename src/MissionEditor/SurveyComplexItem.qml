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

/// Survey Complex Mission Item visuals
Item {
    property var map    ///< Map control to place item in

    property var _polygon
    property var _grid

    Component.onCompleted: {
        _polygon = polygonComponent.createObject(map)
        _grid = gridComponent.createObject(map)
        map.addMapItem(_polygon)
        map.addMapItem(_grid)
    }

    Component.onDestruction: {
        _polygon.destroy()
        _grid.destroy()
    }

    // Survey area polygon
    Component {
        id: polygonComponent

        MapPolygon {
            color: "green"
            opacity:    0.5
            path:       object.polygonPath
        }
    }

    // Survey grid lines
    Component {
        id: gridComponent

        MapPolyline {
            line.color: "white"
            line.width: 2
            path:       object.gridPoints
        }
    }
}
