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
import QGroundControl.GeoMap

/// Fixed Wing landing pattern visual for GeoMap: the shared landing-pattern
/// base plus the FW-only landing-area/glide-slope polygons, labels and
/// height annotations (src/PlanView/FWLandingPatternMapVisual.qml). Polygon
/// shapes are exact
/// geo-math ports (great-circle distance/azimuth, camera-orientation
/// independent); label rotation uses on-screen bearing instead of geographic
/// azimuth since GeoMap's 2D camera heading is user-rotatable (see
/// GeoMapMissionArrow for the same adaptation).
GeoMapLandingPatternVisuals {
    id: root

    readonly property real _landingWidthMeters: 15
    readonly property real _landingLengthMeters: 100
    readonly property real _landingAreaBearing: item.landingCoordinate.azimuthTo(item.slopeStartCoordinate)

    readonly property real _angleRadians: Math.atan((_landingWidthMeters / 2) / (_landingLengthMeters / 2))
    readonly property real _angleDegrees: _angleRadians * (180 / Math.PI)
    readonly property real _hypotenuse: (_landingWidthMeters / 2) / Math.sin(_angleRadians)

    readonly property var _landingAreaCorner1: item.landingCoordinate.atDistanceAndAzimuth(_hypotenuse, _landingAreaBearing - _angleDegrees)
    readonly property var _landingAreaCorner2: item.landingCoordinate.atDistanceAndAzimuth(_hypotenuse, _landingAreaBearing + _angleDegrees)
    readonly property var _landingAreaCorner3: item.landingCoordinate.atDistanceAndAzimuth(_hypotenuse, _landingAreaBearing + (180 - _angleDegrees))
    readonly property var _landingAreaCorner4: item.landingCoordinate.atDistanceAndAzimuth(_hypotenuse, _landingAreaBearing - (180 - _angleDegrees))
    readonly property var _glideSlopeThirdCoordinate: _useLoiterToAlt ? item.slopeStartCoordinate : item.finalApproachCoordinate

    GeoMapProjectedPath {
        id: landingAreaPath
        scene: root.scene
        surfaceModel: root.surfaceModel
        // First corner repeated: PathPolyline strokes an open path, so the
        // outline needs the closing edge spelled out
        coordinates: [root._landingAreaCorner1, root._landingAreaCorner2, root._landingAreaCorner3, root._landingAreaCorner4, root._landingAreaCorner1]
    }

    // The source gates these FW-only elements on isCurrentItem (the item
    // selected in the Plan editor); in the read-only fly view an item is only
    // "current" while the vehicle is executing that exact sequence number, so
    // that gate would hide them essentially always. Show them whenever the
    // pattern is set instead.
    readonly property bool _showExtendedVisuals: item.landingCoordSet

    Shape {
        visible: root._showExtendedVisuals && landingAreaPath.projected
        ShapePath {
            strokeColor: "black"
            strokeWidth: 1
            // ShapePath has no fillOpacity; bake the alpha into the fill color
            fillColor: Qt.alpha("green", 0.5)
            PathPolyline { path: landingAreaPath.screenPoints }
        }
    }

    GeoMapProjectedPath {
        id: glideSlopePath
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        // Unlike the flat 2D source, render the slope as a true 3D surface:
        // base edge at the transition altitude over the landing-area edge
        // (the slope's actual crossing height there), apex up at the final
        // approach altitude
        coordinates: [
            QtPositioning.coordinate(root._landingAreaCorner1.latitude, root._landingAreaCorner1.longitude,
                                     root._landAltAMSL + root._transitionAltitudeMeters),
            QtPositioning.coordinate(root._landingAreaCorner2.latitude, root._landingAreaCorner2.longitude,
                                     root._landAltAMSL + root._transitionAltitudeMeters),
            QtPositioning.coordinate(root._glideSlopeThirdCoordinate.latitude, root._glideSlopeThirdCoordinate.longitude,
                                     root._approachAltAMSL),
            // Repeat the first base corner: PathPolyline strokes an open path
            QtPositioning.coordinate(root._landingAreaCorner1.latitude, root._landingAreaCorner1.longitude,
                                     root._landAltAMSL + root._transitionAltitudeMeters)
        ]
    }

    Shape {
        visible: root._showExtendedVisuals && glideSlopePath.projected
        ShapePath {
            strokeColor: "black"
            strokeWidth: 1
            fillColor: Qt.alpha(root.item.terrainCollision ? "red" : "orange", 0.5)
            PathPolyline { path: glideSlopePath.screenPoints }
        }
    }

    // Screen-space bearing (0 = up, clockwise) of the landing-area direction,
    // folded the same way the source folds the geographic azimuth so the
    // label text never renders upside down
    function _adjustedBearing(rawBearing) {
        let adjusted = rawBearing < 0 ? rawBearing + 360 : rawBearing
        if (adjusted > 180) {
            adjusted -= 180
        }
        adjusted -= 90
        if (adjusted < 0) {
            adjusted += 360
        }
        return adjusted
    }

    GeoMapProjectedPath {
        id: bearingPath
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinates: [root.item.landingCoordinate, root.item.slopeStartCoordinate]
    }

    // Normalized to [0, 360) to match the source visual's geographic-azimuth
    // conventions (atan2 alone yields [-180, 180])
    readonly property real _screenBearing: {
        if (!bearingPath.projected || bearingPath.screenPoints.length < 2) {
            return 0
        }
        const bearing = Math.atan2(bearingPath.screenPoints[1].x - bearingPath.screenPoints[0].x,
                                   -(bearingPath.screenPoints[1].y - bearingPath.screenPoints[0].y)) * 180 / Math.PI
        return bearing < 0 ? bearing + 360 : bearing
    }

    GeoMapItem {
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.ClampToGround
        coordinate: root.item.landingCoordinate
        visible: root._showExtendedVisuals
        anchorPoint: Qt.point(landingAreaLabel.width / 2, landingAreaLabel.height / 2)
        width: landingAreaLabel.width
        height: landingAreaLabel.height

        QGCLabel {
            id: landingAreaLabel
            text: qsTr("Landing Area")
            color: "white"
            rotation: root._adjustedBearing(root._screenBearing)
        }
    }

    GeoMapItem {
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.ClampToGround
        coordinate: root.item.landingCoordinate.atDistanceAndAzimuth(root._landingLengthMeters / 2 + 2, root._landingAreaBearing)
        visible: root._showExtendedVisuals
        anchorPoint: Qt.point(root._screenBearing > 180 ? glideSlopeLabel.width : 0, glideSlopeLabel.height / 2)
        width: glideSlopeLabel.width
        height: glideSlopeLabel.height

        QGCLabel {
            id: glideSlopeLabel
            text: qsTr("Glide Slope")
            color: "white"
            rotation: root._adjustedBearing(root._screenBearing)
        }
    }

    readonly property real _finalApproachAltitudeMeters: item.finalApproachAltitude.rawValue
    readonly property real _landingAltitudeMeters: item.landingAltitude.rawValue
    readonly property real _glideSlopeAdjacent: _useLoiterToAlt ? item.landingCoordinate.distanceTo(item.slopeStartCoordinate)
                                                                 : item.landingCoordinate.distanceTo(item.finalApproachCoordinate)
    readonly property real _glideSlopeAngleRadians: Math.atan((_finalApproachAltitudeMeters - _landingAltitudeMeters) / _glideSlopeAdjacent)
    readonly property real _transitionDistance: _landingLengthMeters / 2
    readonly property real _transitionAltitudeMeters: Math.tan(_glideSlopeAngleRadians) * _transitionDistance
    readonly property real _midSlopeAltitudeMeters: Math.tan(_glideSlopeAngleRadians) * (_transitionDistance + ((_glideSlopeAdjacent - _transitionDistance) / 2))

    readonly property int _angleIncrement: _landingAreaBearing > 180 ? -90 : 90
    readonly property var _transitionCoordinate: item.landingCoordinate.atDistanceAndAzimuth(_landingLengthMeters / 2, _landingAreaBearing)
    readonly property var _midSlopeCenterCoordinate: _transitionCoordinate.atDistanceAndAzimuth(
        _transitionCoordinate.distanceTo(_glideSlopeThirdCoordinate) / 2, _landingAreaBearing)

    GeoMapMissionHeightBadge {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._transitionCoordinate.atDistanceAndAzimuth(root._landingWidthMeters, root._landingAreaBearing + root._angleIncrement)
        visible: root._showExtendedVisuals
        text: Math.floor(QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(root._transitionAltitudeMeters)) + " "
              + QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
    }

    GeoMapMissionHeightBadge {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._midSlopeCenterCoordinate.atDistanceAndAzimuth(root._landingWidthMeters / 2, root._landingAreaBearing + root._angleIncrement)
        visible: root._showExtendedVisuals
        text: Math.floor(QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(root._midSlopeAltitudeMeters)) + " "
              + QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
    }

    GeoMapMissionHeightBadge {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root.item.slopeStartCoordinate
        visible: root._showExtendedVisuals
        text: root.item.finalApproachAltitude.value.toFixed(1) + " " + root.item.finalApproachAltitude.units
    }
}
