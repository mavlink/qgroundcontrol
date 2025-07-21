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
                var segmentDist = (i * distance) / numSegments
                var interpolatedCoord = coord1.atDistanceAndAzimuth(segmentDist, coord1.azimuthTo(coord2))
                pathPoints.push(interpolatedCoord)
            }

            pathPoints.push(coord2)
            return pathPoints
        }
    }
}
