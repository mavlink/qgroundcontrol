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
import QtQuick.Controls 1.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0

/// Handles adding and removing vehicle indicators to a FlightMap control
Item {
    property var map    ///< Map control this item interacts with

    property var _vehicles: []          ///< List of known vehicles
    property var _vehicleMapItems: []   ///< List of known vehicle map items

    Component.onCompleted: addExistingVehicles()

    Connections {
        target: multiVehicleManager

        onVehicleAdded:     addVehicle(vehicle)
        onVehicleRemoved:   removeVehicle(vehicle)
    }

    function addVehicle(vehicle) {
        var qmlItemTemplate = "VehicleMapItem { " +
                                    "coordinate:    vehicles[%1].coordinate; " +
                                    "heading:       vehicles[%1].heading " +
                                "}"

        var i = _vehicles.length
        qmlItemTemplate = qmlItemTemplate.replace("%1", i)
        qmlItemTemplate = qmlItemTemplate.replace("%1", i)

        _vehicles.push(vehicle)
        var mapItem = Qt.createQmlObject (qmlItemTemplate, map)
        _vehicleMapItems.push(mapItem)

        mapItem.z = map.z + 1
        map.addMapItem(mapItem)
    }

    function removeVehicle(vehicle) {
        for (var i=0; i<_vehicles.length; i++) {
            if (_vehicles[i] == vehicle) {
                _vehicle[i] = undefined
                map.removeMapItem(_vehicleMapItems[i])
                _vehicleMapItems[i] = undefined
                break
            }
        }
    }

    function addExistingVehicles() {
        for (var i=0; i<multiVehicleManager.vehicles.length; i++) {
            addVehicle(multiVehicleManager.vehicles[i])
        }
    }
}
