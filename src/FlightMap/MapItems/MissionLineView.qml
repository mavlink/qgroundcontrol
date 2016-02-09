/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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
