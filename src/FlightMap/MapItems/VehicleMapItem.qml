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

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick          2.4
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

/// Marker for displaying a vehicle location on the map
MapQuickItem {
    property var    vehicle                ///< Vehicle object
    property bool   isSatellite:    false  ///< true: satellite map is showing
    property real   size:           ScreenTools.defaultFontPixelHeight * 5

    anchorPoint.x:  vehicleIcon.width  / 2
    anchorPoint.y:  vehicleIcon.height / 2
    visible:        vehicle && vehicle.coordinateValid

    sourceItem: Image {
        id:         vehicleIcon
        source:     isSatellite ? "/qmlimages/airplaneOpaque.svg" : "/qmlimages/airplaneOutline.svg"
        mipmap:     true
        width:      size
        fillMode:   Image.PreserveAspectFit

        transform: Rotation {
            origin.x:   vehicleIcon.width  / 2
            origin.y:   vehicleIcon.height / 2
            angle:      vehicle ? vehicle.heading.value : 0
        }
    }
}
