/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                          2.11

import QGroundControl                   1.0
import QGroundControl.SettingsManager   1.0

Canvas {
    id: canvas
    anchors.fill: parent
    visible: QGroundControl.settingsManager.flyViewSettings.showObstacleDistanceOverlay.value > 0 && _activeVehicle && _activeVehicle.objectAvoidance.available

    property var showText: true
    property var interval: 200
    property var _ranges: []
    property var _incrementDeg: 0
    property var _offsetDeg: 0
    property var _heading: 0
    property var _maxRadiusMeters: 0
    property var _rangesLen: 0
    property var _degToRangeIdx: function(deg, useHeading) {
        return rangeIdx(deg, _incrementDeg, _offsetDeg, _rangesLen, useHeading ? _heading : 0)
    }
    property var _rangeToShow: function(range) {
        const feets = QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsFeet
        range = feets ? range * 3.2808399 : range
        return range.toFixed(2)
    }

    // Converts degrees to index from ranges
    function rangeIdx(deg, increment, offset, len, heading) {
        const i = (360 - heading + deg - offset) / increment
        return (len + Math.ceil(i)) % len
    }

    Timer {
        interval: canvas.interval
        running: true
        repeat: true
        onTriggered: canvas.requestPaint()
    }

    onPaint: {
        if (!_activeVehicle)
            return

        var ctx = getContext("2d");
        ctx.reset()

        _ranges = _activeVehicle.objectAvoidance.distances
        _incrementDeg = _activeVehicle.objectAvoidance.increment
        if (_ranges.length == 0 || _incrementDeg == 0)
            return

        _offsetDeg = _activeVehicle.objectAvoidance.angleOffset
        _heading = _activeVehicle.heading.value
        _maxRadiusMeters = _activeVehicle.objectAvoidance.maxDistance / 100
        _rangesLen = 360.0 / _incrementDeg
        if (_rangesLen > _ranges.length)
            _rangesLen = _ranges.length

        paintObstacleOverlay(ctx)
    }
}
