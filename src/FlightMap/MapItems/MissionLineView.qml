/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Palette

/// The MissionLineView control is used to add lines between mission items
MapItemView {
    property bool showSpecialVisual: false
    delegate: MapPolyline {
        line.width: 3
        // Note: Special visuals for ROI are hacked out for now since they are not working correctly
        line.color: _terrainCollision ?
                        "red" :
                        (false/*showSpecialVisual*/ ? "green" : QGroundControl.globalPalette.mapMissionTrajectory)
        z:          QGroundControl.zOrderWaypointLines
        path:       _calcMissionLinePath()

        property bool _terrainCollision:    object && object.terrainCollision
        property bool _showSpecialVisual:   object && showSpecialVisual && object.specialVisual

        readonly property real _maxSegmentLengthM: 50000 // 50 km

        function _calcMissionLinePath() {
            if (!object || !object.coordinate1.isValid || !object.coordinate2.isValid) {
                return []
            }

            var coord1 = object.coordinate1
            var coord2 = object.coordinate2

            var distance = coord1.distanceTo(coord2)
            if (distance <= _maxSegmentLengthM) {
                return [coord1, coord2]
            }

            // For longer distances, draw great circle path
            var pathPoints = [coord1]
            var numSegments = Math.ceil(distance / _maxSegmentLengthM)

            for (var i = 1; i < numSegments; i++) {
                var fraction = i / numSegments
                var interpolatedCoord =
                        _interpolateGreatCircle(coord1, coord2, fraction)
                pathPoints.push(interpolatedCoord)
            }

            pathPoints.push(coord2)
            return pathPoints
        }

        function _interpolateGreatCircle(coord1, coord2, fraction) {
            var lat1 = coord1.latitude * Math.PI / 180
            var lon1 = coord1.longitude * Math.PI / 180
            var lat2 = coord2.latitude * Math.PI / 180
            var lon2 = coord2.longitude * Math.PI / 180

            var angularDistance = Math.acos(
                Math.sin(lat1) * Math.sin(lat2) +
                Math.cos(lat1) * Math.cos(lat2) * Math.cos(lon2 - lon1)
            )

            var a = Math.sin((1 - fraction) * angularDistance) /
                    Math.sin(angularDistance)
            var b = Math.sin(fraction * angularDistance) /
                    Math.sin(angularDistance)

            var x = a * Math.cos(lat1) * Math.cos(lon1) +
                    b * Math.cos(lat2) * Math.cos(lon2)
            var y = a * Math.cos(lat1) * Math.sin(lon1) +
                    b * Math.cos(lat2) * Math.sin(lon2)
            var z = a * Math.sin(lat1) + b * Math.sin(lat2)

            var interpolatedLat =
                    Math.atan2(z, Math.sqrt(x * x + y * y)) * 180 / Math.PI
            var interpolatedLon = Math.atan2(y, x) * 180 / Math.PI

            return QtPositioning.coordinate(interpolatedLat,
                                            interpolatedLon,
                                            coord1.altitude)
        }
    }
}
