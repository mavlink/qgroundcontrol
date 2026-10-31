import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.GeoMap

/// Rally point marker. A PX4 rally point is a position on the ground (its altitude is the ground elevation), so it lies
/// flat on the terrain. Other firmware flies to the point's altitude, so the marker floats there.
GeoMapMissionLabel {
    id: root

    property var rallyPoint                 ///< RallyPoint
    property var vehicle                    ///< Vehicle whose firmware decides the meaning of the altitude
    property real homeAltitude: NaN         ///< Home AMSL, for the relative altitude frame
    property real homeTerrainBias: 0        ///< See GeoMapWaypointItem.homeTerrainBias

    readonly property bool onGround: vehicle ? vehicle.px4Firmware : false

    // terrainHeightAt does not notify, so _terrainRev re-evaluates a terrain frame altitude as tiles arrive
    property int _terrainRev: 0
    readonly property bool _terrainFrame: !!rallyPoint && rallyPoint.altitudeFrame === QGroundControl.AltitudeFrameTerrain

    // Rendered height: the rendered terrain is homeTerrainBias above AMSL
    readonly property real _renderedAltitude: {
        root._terrainRev
        if (!rallyPoint) {
            return NaN
        }
        const altitude = rallyPoint.coordinate.altitude
        switch (rallyPoint.altitudeFrame) {
        case QGroundControl.AltitudeFrameAbsolute:
            return altitude + homeTerrainBias
        case QGroundControl.AltitudeFrameTerrain:
            return (surfaceModel ? surfaceModel.terrainHeightAt(rallyPoint.coordinate) : 0) + altitude
        default:
            return homeAltitude + altitude + homeTerrainBias
        }
    }

    objectName: "geoMapRallyPoint"
    label: qsTr("R", "rally point map item label")
    groundMarker: onGround
    altitudeMode: onGround ? GeoMapItem.ClampToGround : GeoMapItem.Absolute
    dragHeightAt: _terrainFrame && surfaceModel
        ? (position) => surfaceModel.terrainHeightAt(position) + rallyPoint.coordinate.altitude
        : null
    coordinate: {
        if (!rallyPoint) {
            return QtPositioning.coordinate()
        }
        if (onGround) {
            return rallyPoint.coordinate
        }
        return QtPositioning.coordinate(rallyPoint.coordinate.latitude, rallyPoint.coordinate.longitude, _renderedAltitude)
    }

    Connections {
        target: root.surfaceModel
        enabled: !root.onGround && root._terrainFrame

        function onTerrainHeightsChanged() {
            root._terrainRev++
        }
    }
}
