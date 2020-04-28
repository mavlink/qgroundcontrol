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
import QGroundControl.Controls      1.0

/// Corridor Scan Complex Mission Item visuals
TransectStyleMapVisuals {
    polygonInteractive: false

    property bool _currentItem: object.isCurrentItem

    QGCMapPolylineVisuals {
        id:             mapPolylineVisuals
        mapControl:     map
        mapPolyline:    object.corridorPolyline
        interactive:    _currentItem && parent.interactive
        lineWidth:      3
        lineColor:      "#be781c"
        visible:        _currentItem
        opacity:        parent.opacity
    }
}
