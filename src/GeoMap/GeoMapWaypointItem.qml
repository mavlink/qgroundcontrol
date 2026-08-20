/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Simple-mission waypoint marker for the GeoMap: a colored dot that
/// crossfades into a 3D sphere, with an always-visible sequence-number label.
/// Marker color matches the existing 2D FlyView convention (MissionItemIndexLabel):
/// current item highlighted, everything else a plain indicator color.
/// Renders nothing for complex mission items, the home item, or items with
/// no real coordinate (e.g. RTL, which flies to home dynamically rather than
/// a mission-encoded lat/lon) — see visible.
GeoMapItem {
    id: root

    property var item          ///< VisualMissionItem for this marker
    // DEM-vs-home-AMSL bias (see GeoMapVehicleItem), computed once by the
    // consumer for the active vehicle and shared across every marker instead
    // of each one re-sampling the same DEM lookup independently
    property real homeTerrainBias: 0
    property real size: ScreenTools.defaultFontPixelHeight * 1.5

    width: size
    height: size
    anchorPoint: Qt.point(width / 2, height / 2)
    altitudeMode: GeoMapItem.Absolute
    // item.amslEntryAlt is AMSL per QGC's own terrain-query pipeline, which
    // can disagree with GeoMap's own DEM tiles; bias-corrected the same way
    // GeoMapVehicleItem aligns the vehicle marker to the rendered mesh
    coordinate: item ? QtPositioning.coordinate(item.coordinate.latitude,
                                                item.coordinate.longitude,
                                                item.amslEntryAlt + homeTerrainBias)
                     : QtPositioning.coordinate()
    visible: item ? item.isSimpleItem && item.specifiesCoordinate : false

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    // Matches MissionItemIndicator.qml's _isCurrentItem exactly: current item
    // (or a complex item with a current child) is highlighted
    readonly property bool _isCurrentItem: item ? (item.isCurrentItem || item.hasCurrentChildItem) : false

    // Matches MissionItemIndexLabel.qml's existing convention: current item
    // is "green", everything else is the plain map indicator color
    readonly property color _markerColor: root._isCurrentItem ? "green" : qgcPal.mapIndicator

    // MissionItemIndexLabel also grows the indicator for the current item
    // (small: !checked); mirrored here for both the 2D dot and 3D sphere
    readonly property real _indicatorSize: root._isCurrentItem ? root.size : root.size * 0.75

    readonly property var _camera: scene ? scene.camera : null

    // Constant apparent size, same technique as GeoMapVehicleItem: scene
    // units per screen pixel at the camera's look-at distance. Built-in
    // QtQuick3D meshes are 100 units across.
    readonly property real _modelScale: _camera
        ? root._indicatorSize * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 1

    // Rendered-terrain height (true meters) under the marker, for the drop
    // line; terrainHeightAt is not a binding dependency, so re-sampled
    // explicitly as terrain tiles arrive and when the item moves
    property real _groundHeight: 0

    function _updateGroundHeight() {
        _groundHeight = (root.surfaceModel && root.item) ? root.surfaceModel.terrainHeightAt(root.item.coordinate) : 0
    }

    Component.onCompleted: _updateGroundHeight()
    onSurfaceModelChanged: _updateGroundHeight()

    Connections {
        target: root.surfaceModel
        function onTerrainHeightsChanged() { root._updateGroundHeight() }
    }

    Connections {
        target: root.item
        function onCoordinateChanged() { root._updateGroundHeight() }
    }

    // Scene-space length of the drop line from marker to the rendered terrain
    // (tracks terrainScale, so it flattens in lockstep with the anchor z)
    readonly property real _dropLength: {
        if (!scene || !item) {
            return 0
        }
        const drop = (item.amslEntryAlt + homeTerrainBias - _groundHeight) * scene.verticalScale * scene.terrainScale
        return isFinite(drop) ? Math.max(0, drop) : 0
    }

    // Drop line diameter: constant apparent width, like the marker
    readonly property real _dropLineScale: _camera
        ? (ScreenTools.defaultFontPixelHeight / 8) * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 0

    Rectangle {
        anchors.centerIn: parent
        width: root._indicatorSize
        height: root._indicatorSize
        radius: width / 2
        color: root._markerColor
        opacity: root.contentOpacity2D
    }

    // Matches MissionItemIndicator.qml's "extra circle to indicate selection"
    Rectangle {
        anchors.centerIn: parent
        width: root._indicatorSize * 1.4
        height: width
        radius: width / 2
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.5)
        border.width: 1
        opacity: root.contentOpacity2D
        visible: root._isCurrentItem
    }

    QGCLabel {
        anchors.centerIn: parent
        text: root.item ? root.item.sequenceNumber : ""
        color: "white"
        font.bold: true
    }

    delegate3D: Component {
        Node {
            Node {
                scale: Qt.vector3d(root._modelScale, root._modelScale, root._modelScale)

                Model {
                    source: "#Sphere"

                    materials: PrincipledMaterial {
                        baseColor: root._markerColor
                    }
                }
            }

            // Drop line to the terrain below (Google Earth-style "extrude"):
            // anchors the floating marker visually so its altitude is readable
            Model {
                source: "#Cylinder"   // built-in: 100x100 units, height along +Y
                visible: root._dropLength > 0
                opacity: 0.75
                eulerRotation.x: 90   // stand the height axis up along scene z
                position: Qt.vector3d(0, 0, -root._dropLength / 2)
                scale: Qt.vector3d(root._dropLineScale, root._dropLength / 100, root._dropLineScale)

                materials: PrincipledMaterial {
                    lighting: PrincipledMaterial.NoLighting
                    baseColor: root._markerColor
                }
            }

            // Ground footprint: half-buried flattened sphere reads as a disc
            // without z-fighting the terrain mesh
            Model {
                source: "#Sphere"
                visible: root._dropLength > 0
                opacity: 0.75
                position: Qt.vector3d(0, 0, -root._dropLength)
                scale: Qt.vector3d(root._modelScale, root._modelScale, root._modelScale * 0.15)

                materials: PrincipledMaterial {
                    lighting: PrincipledMaterial.NoLighting
                    baseColor: root._markerColor
                }
            }
        }
    }
}
