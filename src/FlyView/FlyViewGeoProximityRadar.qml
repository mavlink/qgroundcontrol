/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Shapes
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.GeoMap

/// Proximity sensor sector arcs around a vehicle: GeoMap-engine port of
/// ProximityRadarMapView. The meters-to-pixels ratio is sampled through the
/// camera pick ray at the vehicle's screen position instead of QtLocation's
/// map-wide zoom-level sampling.
GeoMapItem {
    id: root

    property var vehicle
    property double heading: vehicle ? vehicle.heading.value : Number.NaN

    coordinate: vehicle ? vehicle.coordinate : QtPositioning.coordinate()
    visible: proximityValues.telemetryAvailable
    width: detectionLimitCircle.width
    height: detectionLimitCircle.height
    anchorPoint: Qt.point(width / 2, height / 2)

    property real _ratio: 1

    // Density-independent stroke width for the sensor arcs and limit circle
    readonly property real _strokeWidth: ScreenTools.defaultFontPixelHeight / 4

    // Each of the 8 sensor sectors covers 45 degrees, centered on its rotation direction.
    // Sector 0 is vehicle-forward, which is -90 degrees in PathAngleArc coordinates.
    readonly property real _sectorSweepAngle: 360 / 8
    readonly property real _firstSectorStartAngle: -90 - (_sectorSweepAngle / 2)

    readonly property var _camera: scene ? scene.camera : null

    // Cap the rendered circle size to keep item geometry sane when zoomed way in. The clamp only
    // engages once the circle's rim is far outside the viewport, so the clamped (wrong-sized)
    // portion is offscreen and never visible.
    readonly property real _maximumDiameter: _camera ? Math.max(_camera.viewportSize.width, _camera.viewportSize.height) * 4 : 4096

    function calcSize() {
        if (!root._camera) {
            root._ratio = 0
            return
        }
        // Ground meters per pixel at this item's own screen position: two pick
        // rays 100 px apart (stays accurate under 3D camera tilt, where the
        // scale varies across the viewport)
        const samplePixelLength = 100
        const screenPos = Qt.point(root.x + root.anchorPoint.x, root.y + root.anchorPoint.y)
        const leftCoord = root._camera.coordinateAtScreenPoint(screenPos)
        const rightCoord = root._camera.coordinateAtScreenPoint(Qt.point(screenPos.x + samplePixelLength, screenPos.y))
        if (!leftCoord.isValid || !rightCoord.isValid) {
            root._ratio = 0
            return
        }
        const sampleMeters = leftCoord.distanceTo(rightCoord)
        if (!isFinite(sampleMeters) || sampleMeters <= 0) {
            root._ratio = 0
            return
        }
        root._ratio = samplePixelLength / sampleMeters
    }

    function _clampedDiameter(requestedDiameter) {
        if (!isFinite(requestedDiameter) || requestedDiameter < 0) {
            return 0
        }
        return Math.min(requestedDiameter, _maximumDiameter)
    }

    function _sectorRadius(sectorIndex) {
        const sectorDistance = proximityValues.rgRotationValues[sectorIndex]
        // Clamp for the same reason as _clampedDiameter: keep geometry coordinates sane at deep zoom
        return isNaN(sectorDistance) ? 0 : _clampedDiameter(sectorDistance * _ratio * 2) / 2
    }

    function _sectorColor(sectorIndex) {
        return isNaN(proximityValues.rgRotationValues[sectorIndex]) ? Qt.rgba(0, 0, 0, 0) : Qt.rgba(1, 0, 0, 1)
    }

    function _sectorStartAngle(sectorIndex) {
        return _firstSectorStartAngle + (sectorIndex * _sectorSweepAngle)
    }

    ProximityRadarValues {
        id: proximityValues
        vehicle: root.vehicle
    }

    Connections {
        target: root._camera
        function onDistanceChanged() { scaleTimer.restart() }
        function onCenterChanged() { scaleTimer.restart() }
        function onTiltChanged() { scaleTimer.restart() }
        function onHeadingChanged() { scaleTimer.restart() }
        function onViewportSizeChanged() { scaleTimer.restart() }
    }

    // Under 3D tilt the scale varies across the viewport, so vehicle motion
    // alone can invalidate the ratio even with a stationary camera
    Connections {
        target: root.vehicle
        function onCoordinateChanged() { scaleTimer.restart() }
    }

    Timer {
        id: scaleTimer
        interval: 100
        running: false
        repeat: false
        onTriggered: root.calcSize()
    }

    Item {
        id: vehicleItem
        width: detectionLimitCircle.width
        height: detectionLimitCircle.height
        opacity: 0.5 * root.contentOpacity2D

        Component.onCompleted: root.calcSize()

        // Sensor arcs are drawn with Shape rather than Canvas since Shape renders as scene graph
        // geometry and doesn't require a backing store allocation which scales with item size.
        // Each 45 degree sector is centered on its rotation direction: sector 0 is vehicle-forward.
        Shape {
            id: vehicleSensors
            anchors.fill: detectionLimitCircle

            transform: Rotation {
                origin.x: detectionLimitCircle.width / 2
                origin.y: detectionLimitCircle.height / 2
                // Camera heading rotates map north on screen; the sectors follow
                angle: (isNaN(root.heading) ? 0 : root.heading) + (root._camera ? root._camera.heading : 0)
            }

            // ShapePath is not an Item so a Repeater can't be used; an Instantiator which appends
            // to the Shape's data list creates the equivalent of one ShapePath per sensor sector.
            Instantiator {
                model: 8

                delegate: ShapePath {
                    required property int index

                    strokeColor: root._sectorColor(index)
                    strokeWidth: root._strokeWidth
                    fillColor: "transparent"

                    PathAngleArc {
                        centerX: vehicleSensors.width / 2
                        centerY: vehicleSensors.height / 2
                        radiusX: root._sectorRadius(index)
                        radiusY: radiusX
                        startAngle: root._sectorStartAngle(index)
                        sweepAngle: root._sectorSweepAngle
                    }
                }

                onObjectAdded: (index, object) => vehicleSensors.data.push(object)
            }
        }

        Rectangle {
            id: detectionLimitCircle
            width: root._clampedDiameter(proximityValues.maxDistance * 2 * root._ratio)
            height: width
            color: Qt.rgba(1, 1, 1, 0)
            border.color: Qt.rgba(1, 1, 1, 1)
            border.width: root._strokeWidth
            radius: width * 0.5
        }
    }
}
