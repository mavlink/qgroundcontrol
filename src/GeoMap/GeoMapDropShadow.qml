/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Draped ground shadow under a floating marker's drop line: GeoMapCircle
/// clamps each rim point to the terrain mesh, so the shadow follows slopes
/// instead of half-burying like a flat disc. Instantiate as a direct child
/// of the marker's GeoMapItem — the item repositions itself in viewport
/// coordinates and this undoes that offset (screen-space geometry).
/// Also owns the marker-to-ground drop computation (dropLength /
/// dropLineScale) shared with the delegate3D's GeoMapDropLine, since the
/// shadow already samples the terrain under the marker.
GeoMapCircle {
    id: root

    property var geoItem            ///< Owning GeoMapItem (the marker)
    property real indicatorSize: 0  ///< Marker diameter in pixels; sets the shadow radius

    /// Scene-space length of the drop line from marker to the rendered
    /// terrain (tracks terrainScale, so it flattens in lockstep with the
    /// anchor z); 0 for clamped markers, so they show no line
    readonly property real dropLength: {
        if (!scene || !geoItem || geoItem.altitudeMode !== GeoMapItem.Absolute || isNaN(geoItem.coordinate.altitude)) {
            return 0
        }
        const drop = (geoItem.coordinate.altitude - _groundHeight) * scene.verticalScale * scene.terrainScale
        return isFinite(drop) ? Math.max(0, drop) : 0
    }

    /// Drop line diameter: constant apparent width, like the marker
    /// (built-in QtQuick3D meshes are 100 units across)
    readonly property real dropLineScale: _camera
        ? (ScreenTools.defaultFontPixelHeight / 8) * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 0

    readonly property var _camera: scene ? scene.camera : null

    // Rendered-terrain height (true meters) under the marker; terrainHeightAt
    // is not a binding dependency, so re-sampled explicitly as terrain tiles
    // arrive and when the marker moves. Keyed on lat/lon so altitude-only
    // coordinate changes (e.g. terrain-bias updates) skip the resample.
    property real _groundHeight: 0
    readonly property real _latitude: geoItem ? geoItem.coordinate.latitude : NaN
    readonly property real _longitude: geoItem ? geoItem.coordinate.longitude : NaN

    function _updateGroundHeight() {
        _groundHeight = (surfaceModel && geoItem && geoItem.coordinate.isValid)
            ? surfaceModel.terrainHeightAt(geoItem.coordinate) : 0
    }

    Component.onCompleted: _updateGroundHeight()
    onSurfaceModelChanged: _updateGroundHeight()
    on_LatitudeChanged: _updateGroundHeight()
    on_LongitudeChanged: _updateGroundHeight()

    Connections {
        target: root.surfaceModel
        function onTerrainHeightsChanged() { root._updateGroundHeight() }
    }

    x: geoItem ? -geoItem.x : 0
    y: geoItem ? -geoItem.y : 0
    visible: dropLength > 0
    opacity: scene ? scene.terrainScale : 0
    scene: geoItem ? geoItem.scene : null
    surfaceModel: geoItem ? geoItem.surfaceModel : null
    center: geoItem ? geoItem.coordinate : QtPositioning.coordinate()
    radiusMeters: _camera
        ? (indicatorSize / 2) * _camera.distance * _camera.unitsPerPixelAtUnitDistance
        : 1
    strokeColor: "transparent"
    strokeWidth: 0
    // Translucent black (ADS-B shadow convention) so the colored drop line
    // stays visible where it meets the shadow
    fillColor: Qt.alpha("black", 0.35)
}
