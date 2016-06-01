/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl           1.0
import QGroundControl.Palette   1.0

/// The MissionLineView control is used to add lines between mission items
MapItemView {
    id: _root

    property bool homePositionValid: true   ///< true: show home position, false: don't show home position

    delegate: MapPolyline {
        line.width: 3
        line.color: "#be781c"                           // Hack, can't get palette to work in here
        z:          QGroundControl.zOrderMapItems - 1   // Under item indicators

        path: [
            { latitude: object.coordinate1.latitude, longitude: object.coordinate1.longitude },
            { latitude: object.coordinate2.latitude, longitude: object.coordinate2.longitude },
        ]
    }
}
