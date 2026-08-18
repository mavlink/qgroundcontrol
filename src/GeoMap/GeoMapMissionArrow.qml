/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

/// Direction-indicator chevron for complex item visuals, positioned and
/// rotated in screen space (not anchored via GeoMapItem): GeoMap's 2D camera
/// heading is user-rotatable, so a geographic azimuth (as used by
/// src/FlightMap/MapItems/MapLineArrow.qml) would point the wrong way on
/// screen once the map is rotated. Callers derive screenPoint/heading from
/// already-projected GeoMapProjectedPath.screenPoints (see
/// GeoMapSegmentArrow.qml), avoiding a second projector per arrow.
Canvas {
    id: root

    property point screenPoint: Qt.point(0, 0)
    property real heading: 0    ///< degrees, 0 = up, clockwise

    readonly property real _arrowSize: 15

    x: screenPoint.x - width / 2
    y: screenPoint.y
    width: _arrowSize * 2
    height: _arrowSize
    rotation: heading
    transformOrigin: Item.Top

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        ctx.lineWidth = 2
        ctx.strokeStyle = "white"
        ctx.beginPath()
        ctx.moveTo(_arrowSize, 0)
        ctx.lineTo(_arrowSize * 2, _arrowSize)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(_arrowSize, 0)
        ctx.lineTo(0, _arrowSize)
        ctx.stroke()
    }

    Component.onCompleted: requestPaint()
}
