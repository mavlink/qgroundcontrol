/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtLocation               5.3
import QtPositioning            5.3
import QtGraphicalEffects       1.0

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0

MapPolyline {
    id:             _root
    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    coordinate
    property var    map
    property double heading:    vehicle ? vehicle.heading.value : Number.NaN    ///< Vehicle heading, NAN for none
    property bool   enabled

    visible:    enabled && proximityValues.telemetryAvailable && coordinate.isValid
    line.width: 5
    line.color: Qt.rgba(1,1,1,1)
    opacity:    0.5

    path: coordinate.isValid ? generateOctagon(coordinate, proximityValues) : []

    function generateOctagon(coord, sensor) {
        let path = [];
        let distances = sensor.rgRotationValues;
        let maxDistance = sensor.maxDistance;
        for (let i = 0; i < distances.length; i++) {
            let distance = distances[i];
            if (isNaN(distance)) {
                distance = maxDistance;
            }
            let startAzimuth = heading - 22.5 + i*45.0;
            let endAzimuth = heading - 22.5 + (i+1)*45.0;
            let startCoord = coord.atDistanceAndAzimuth(distance, startAzimuth);
            let endCoord = coord.atDistanceAndAzimuth(distance, endAzimuth);
            path.push(startCoord);
            path.push(endCoord);
        }
        return path;
    }

    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                _root.vehicle
    }

}

