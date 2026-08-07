import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

MapQuickItem {
    id: root

    required property FlightMap mapControl

    property color  arrowColor:     "white"
    property var    fromCoord:      QtPositioning.coordinate()
    property var    toCoord:        QtPositioning.coordinate()
    property int    arrowPosition:  1 ///< 1: first quarter, 2: halfway, 3: last quarter

    property real   _arrowSize:     15
    property real   _arrowHeading:  0
    property real   _screenLegLength: 0

    function _updateArrowDetails() {
        if (fromCoord && fromCoord.isValid && toCoord && toCoord.isValid) {
            var lineDistanceQuarter = fromCoord.distanceTo(toCoord) / 4
            coordinate = fromCoord.atDistanceAndAzimuth(lineDistanceQuarter * arrowPosition, fromCoord.azimuthTo(toCoord))
            _arrowHeading = coordinate.azimuthTo(toCoord) // Account for changing bearing along great circle path
        } else {
            coordinate = QtPositioning.coordinate()
            _arrowHeading = 0
        }
        _updateScreenLegLength()
    }

    function _updateScreenLegLength() {
        if (mapControl && fromCoord && fromCoord.isValid && toCoord && toCoord.isValid) {
            const fromPoint = mapControl.fromCoordinate(fromCoord, false)
            const toPoint = mapControl.fromCoordinate(toCoord, false)
            _screenLegLength = Math.hypot(toPoint.x - fromPoint.x, toPoint.y - fromPoint.y)
        } else {
            _screenLegLength = 0
        }
    }

    onFromCoordChanged: _updateArrowDetails()
    onToCoordChanged:   _updateArrowDetails()
    onMapControlChanged: _updateScreenLegLength()

    Component.onCompleted: _updateArrowDetails()

    Connections {
        target: root.mapControl

        function onCenterChanged() { root._updateScreenLegLength() }
        function onZoomLevelChanged() { root._updateScreenLegLength() }
    }

    sourceItem: Canvas {
        x:      -_arrowSize
        y:      0
        width:  _arrowSize * 2
        height: _arrowSize
        visible: root._screenLegLength >= width

        onPaint: {
            var ctx = getContext("2d");
            ctx.lineWidth = 2
            ctx.strokeStyle = arrowColor
            ctx.beginPath();
            ctx.moveTo(_arrowSize, 0);
            ctx.lineTo(_arrowSize * 2, _arrowSize)
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(_arrowSize, 0);
            ctx.lineTo(0, _arrowSize)
            ctx.stroke();
        }

        transform: Rotation {
            origin.x:   width / 2
            origin.y:   0
            angle:      _arrowHeading
        }
    }
}
