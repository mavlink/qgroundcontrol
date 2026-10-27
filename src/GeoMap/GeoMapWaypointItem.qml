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

import "GeoMapAltitude.js" as GeoMapAltitude

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
    property bool collapsed: false  ///< Drawn by another marker of its overlap group (see GeoMapMissionItems)
    property bool grouped: false    ///< Stands for an overlap group: shows a "…" badge
    property bool draggable: false  ///< The marker can be dragged to move its item; the map is not panned meanwhile
    property bool pickable: true    ///< The marker takes clicks and drags; false lets them through to the map

    signal clicked()
    signal dragged(var coordinate)  ///< Horizontal position the marker was dragged to

    readonly property bool hasMarker: item ? item.isSimpleItem && item.specifiesCoordinate : false
    readonly property real smallIndicatorSize: size * 0.75

    objectName: "geoMapWaypointMarker"
    width: size
    height: size
    anchorPoint: Qt.point(width / 2, height / 2)
    altitudeMode: GeoMapItem.Absolute
    // item.amslEntryAlt is AMSL per QGC's own terrain-query pipeline, which
    // can disagree with GeoMap's own DEM tiles; bias-corrected the same way
    // GeoMapVehicleItem aligns the vehicle marker to the rendered mesh.
    // It is NaN while a terrain-frame item waits on its terrain query.
    // terrainHeightAt does not notify, so _terrainRev re-evaluates the fallback as terrain tiles arrive.
    property int _terrainRev: 0
    readonly property real _pendingTerrainHeight: {
        root._terrainRev
        return item && isNaN(item.amslEntryAlt) && surfaceModel ? surfaceModel.terrainHeightAt(item.coordinate) : NaN
    }
    readonly property real _amsl: !item ? NaN : GeoMapAltitude.displayAmsl(
        item.amslEntryAlt,
        item.isSimpleItem ? item.altitude.rawValue : NaN,
        _pendingTerrainHeight,
        homeTerrainBias)
    coordinate: item ? QtPositioning.coordinate(item.coordinate.latitude,
                                                item.coordinate.longitude,
                                                _amsl + homeTerrainBias)
                     : QtPositioning.coordinate()
    visible: hasMarker && !collapsed

    // Terrain-relative items stand at their height above the terrain, so moving one changes its height
    readonly property bool _terrainRelative: !!item && item.isSimpleItem
        && (item.altitudeFrame === QGroundControl.AltitudeFrameTerrain
            || item.altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain)

    // Coordinate on the horizontal plane at the marker's height that the cursor is over. A terrain-relative marker
    // moves to the height it has at the picked position, so the pick is repeated at that height.
    function _coordinateUnderCursor(target) {
        const camera = root.scene.camera
        let coordinate = camera.coordinateAtScreenPoint(target, root.scenePosition.z)
        if (!root._terrainRelative || !root.surfaceModel) {
            return coordinate
        }
        const sceneScale = root.scene.verticalScale * root.scene.terrainScale
        for (let round = 0; round < 3 && coordinate.isValid; ++round) {
            const height = root.surfaceModel.terrainHeightAt(coordinate) + root.item.altitude.rawValue
            const refined = camera.coordinateAtScreenPoint(target, height * sceneScale)
            if (!refined.isValid) {
                break
            }
            coordinate = refined
        }
        return coordinate
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Connections {
        target: root.surfaceModel
        enabled: !!root.item && isNaN(root.item.amslEntryAlt)

        function onTerrainHeightsChanged() {
            root._terrainRev++
        }
    }

    // Matches MissionItemIndicator.qml's _isCurrentItem exactly: current item
    // (or a complex item with a current child) is highlighted
    readonly property bool _isCurrentItem: item ? (item.isCurrentItem || item.hasCurrentChildItem) : false

    // Matches MissionItemIndexLabel.qml's existing convention: current item
    // is "green", everything else is the plain map indicator color
    readonly property color _markerColor: root._isCurrentItem ? "green" : qgcPal.mapIndicator

    // MissionItemIndexLabel also grows the indicator for the current item
    // (small: !checked) and for a group; mirrored here for both the 2D dot and 3D sphere
    readonly property real _indicatorSize: (root._isCurrentItem || root.grouped) ? root.size : root.smallIndicatorSize

    readonly property var _camera: scene ? scene.camera : null

    // Constant apparent size, same technique as GeoMapVehicleItem: scene
    // units per screen pixel at the camera's look-at distance. Built-in
    // QtQuick3D meshes are 100 units across.
    readonly property real _modelScale: _camera
        ? root._indicatorSize * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 1

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

    QGCLabel {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: "…"
        color: "white"
        font.pointSize: ScreenTools.smallFontPointSize
        visible: root.grouped
    }

    // The item itself tracks the projected marker position at any tilt, so
    // this hit target follows the 3D sphere through the 2D<->3D crossfade
    MouseArea {
        anchors.fill: parent
        enabled: root.pickable
        // The map pan DragHandler would otherwise take over past the drag threshold
        preventStealing: root.draggable

        // Cursor-to-anchor offset at press, so the marker doesn't jump to the cursor
        property point _grabOffset
        property point _pressPosition
        property bool _moved: false

        function _parentPosition(mouse) {
            return root.mapToItem(root.parent, mouse.x, mouse.y)
        }

        onPressed: (mouse) => {
            // Modified drags are the map's orbit and look gestures
            if (mouse.modifiers !== Qt.NoModifier) {
                mouse.accepted = false
                return
            }
            const pos = _parentPosition(mouse)
            _pressPosition = pos
            _grabOffset = Qt.point(root.x + root.anchorPoint.x - pos.x, root.y + root.anchorPoint.y - pos.y)
            _moved = false
        }

        onPositionChanged: (mouse) => {
            if (!root.draggable || !pressed) {
                return
            }
            const pos = _parentPosition(mouse)
            if (!_moved && Math.hypot(pos.x - _pressPosition.x, pos.y - _pressPosition.y) < Qt.styleHints.startDragDistance) {
                return
            }
            _moved = true
            // The marker sits on the horizontal plane at its height, so it stays under the cursor at any tilt
            const target = Qt.point(pos.x + _grabOffset.x, pos.y + _grabOffset.y)
            const coordinate = root._coordinateUnderCursor(target)
            if (coordinate.isValid) {
                root.dragged(coordinate)
            }
        }

        onClicked: {
            if (!_moved) {
                root.clicked()
            }
        }
    }

    GeoMapDropShadow {
        id: dropShadow
        geoItem: root
        indicatorSize: root._indicatorSize
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

            GeoMapDropLine {
                dropLength: dropShadow.dropLength
                lineScale: dropShadow.dropLineScale
                lineColor: root._markerColor
            }
        }
    }
}
