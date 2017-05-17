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

/// QGCMapCircle map visuals
Item {
    id: _root

    property var    mapControl                          ///< Map control to place item in
    property var    mapCircle                           ///< QGCMapCircle object
    property bool   interactive:        false           /// true: user can manipulate polygon
    property color  interiorColor:      "transparent"
    property real   interiorOpacity:    1
    property int    borderWidth:        0
    property color  borderColor:        "black"

    property var _circleComponent

    function addVisuals() {
        _circleComponent = circleComponent.createObject(mapControl)
        mapControl.addMapItem(_circleComponent)
    }

    function removeVisuals() {
        _circleComponent.destroy()
    }

    Component.onCompleted: {
        addVisuals()
    }

    Component.onDestruction: {
        removeVisuals()
    }

    Component {
        id: circleComponent

        MapCircle {
            color:          interiorColor
            opacity:        interiorOpacity
            border.color:   borderColor
            border.width:   borderWidth
            center:         mapCircle.center
            radius:         mapCircle.radius
        }
    }
}

