/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtPositioning
import QtQuick.Shapes

import QGroundControl.GeoMap

/// Base visual shared by Fixed Wing and VTOL landing patterns on GeoMap:
/// read-only GeoMap-native port of the common elements in
/// src/PlanView/FWLandingPatternMapVisual.qml / VTOLLandingPatternMapVisual.qml
/// (flight path, loiter ring, final-approach/landing markers). No
/// mouse-area/drag-to-place code ports over — FlyView missions are already
/// loaded and immutable.
Item {
    id: root

    property var item          ///< VisualMissionItem (LandingComplexItem)
    property var scene
    property var surfaceModel
    // DEM-vs-home-AMSL bias, wired by GeoMapMissionItems (see GeoMapWaypointItem)
    property real homeTerrainBias: 0
    property string landingLabel: ""   ///< Label for the landing-point marker (VTOL overrides to "Land")

    readonly property bool _useLoiterToAlt: item.useLoiterToAlt.rawValue
    // Approach leg descends from the final-approach altitude to the landing
    // altitude; project at those AMSL heights so the 3D view shows the slope
    // instead of a ground-hugging line
    readonly property real _approachAltAMSL: item.amslEntryAlt + homeTerrainBias
    readonly property real _landAltAMSL: item.amslExitAlt + homeTerrainBias
    readonly property var _flightPath: {
        const from = _useLoiterToAlt ? item.slopeStartCoordinate : item.finalApproachCoordinate
        return [QtPositioning.coordinate(from.latitude, from.longitude, _approachAltAMSL),
                QtPositioning.coordinate(item.landingCoordinate.latitude, item.landingCoordinate.longitude, _landAltAMSL)]
    }

    GeoMapProjectedPath {
        id: flightPathProjector
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        coordinates: root._flightPath
    }

    // Flight path (final approach/loiter exit -> landing point)
    Shape {
        visible: root.item.landingCoordSet && flightPathProjector.projected
        ShapePath {
            strokeColor: "#be781c"
            strokeWidth: 2
            fillColor: "transparent"
            PathPolyline { path: flightPathProjector.screenPoints }
        }
    }

    GeoMapProjectedPath {
        id: loiterPath
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        // Static invokables aren't callable via the QML type name; call
        // through the instance instead. Ring altitude rides on the center
        // coordinate (atDistanceAndAzimuth preserves it).
        coordinates: root._useLoiterToAlt
                     ? loiterPath.circleCoordinates(QtPositioning.coordinate(root.item.finalApproachCoordinate.latitude,
                                                                             root.item.finalApproachCoordinate.longitude,
                                                                             root._approachAltAMSL),
                                                    root.item.loiterRadius.rawValue)
                     : []
    }

    // Loiter ring
    Shape {
        visible: root._useLoiterToAlt && loiterPath.projected
        ShapePath {
            strokeColor: "green"
            strokeWidth: 2
            fillColor: "transparent"
            PathPolyline { path: loiterPath.screenPoints }
        }
    }

    // Final approach / loiter point
    GeoMapMissionLabel {
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        coordinate: QtPositioning.coordinate(root.item.finalApproachCoordinate.latitude,
                                             root.item.finalApproachCoordinate.longitude,
                                             root._approachAltAMSL)
        index: root.item.sequenceNumber
        label: root._useLoiterToAlt ? qsTr("Loiter") : qsTr("Approach")
        checked: root.item.isCurrentItem
        visible: root.item.landingCoordSet
    }

    // Landing point
    GeoMapMissionLabel {
        scene: root.scene
        surfaceModel: root.surfaceModel
        altitudeMode: GeoMapItem.Absolute
        coordinate: QtPositioning.coordinate(root.item.landingCoordinate.latitude,
                                             root.item.landingCoordinate.longitude,
                                             root._landAltAMSL)
        index: root.item.lastSequenceNumber
        label: root.landingLabel
        checked: root.item.isCurrentItem
        visible: root.item.landingCoordSet
    }
}
