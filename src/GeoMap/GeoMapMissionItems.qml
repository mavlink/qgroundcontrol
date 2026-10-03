/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.GeoMap

/// Mission-item overlay for the GeoMap: instantiates the GeoMap-native visual
/// declared by each item's VisualMissionItem.geoMapVisualQML (mirrors the
/// existing mapVisualQML dispatch consumed by src/PlanView/MissionItemMapVisual.qml
/// — every item type states its own visual, simple items included, rather
/// than special-casing GeoMapWaypointItem outside the dispatch). Items with
/// no GeoMap-native visual render nothing: the home item (already shown via
/// its own GeoMapPin) and the Survey/Corridor/Structure complex items, which
/// never appear in the fly-view controller (vehicle downloads reconstruct
/// only simple items and landing patterns).
/// Overlapping waypoint markers are grouped (MissionItemIndicatorGroup parity): a
/// click reports the whole group in either mode, and settled top-down 2D also
/// collapses each group into one badged marker.
Item {
    id: root

    property var missionController
    property var scene
    property var surfaceModel
    property real homeTerrainBias: 0   ///< See GeoMapWaypointItem.homeTerrainBias

    /// A marker was clicked; item is its VisualMissionItem (mirrors
    /// MissionItemMapVisual.clicked as consumed by PlanMapItems)
    signal itemClicked(var item)

    /// A marker overlapping others was clicked; items are the group's
    /// VisualMissionItems by sequence number, marker the clicked GeoMapWaypointItem
    signal groupClicked(var items, var marker)

    /// Markers moved on screen (zoom, resize, 2D/3D), so an open group picker is stale.
    /// Center moves don't count: vehicle follow recenters on every position update.
    signal groupsInvalidated()

    readonly property var _camera: scene ? scene.camera : null
    // Top-down 2D only pans, rotates and zooms, so groups change only with the zoom
    readonly property bool _collapse: _camera !== null && _camera.mode === GeoMapCamera.Mode2D && _camera.isTopDown
                                      && scene.terrainScale === 0

    // Re-evaluates when a marker moves or the current item changes
    readonly property var _groupingState: {
        const state = []
        const items = root.missionController ? root.missionController.visualItems : null
        for (let i = 0; items && i < items.count; ++i) {
            const item = items.get(i)
            if (item.isSimpleItem && item.specifiesCoordinate) {
                state.push([item.coordinate, item.isCurrentItem])
            }
        }
        return state
    }

    function _waypointMarkers() {
        const markers = []
        for (let i = 0; i < repeater.count; ++i) {
            const loader = repeater.itemAt(i)
            if (loader && loader.item && loader.item.hasOwnProperty("collapsed")) {
                markers.push(loader.item)
            }
        }
        return markers
    }

    // Groups of on-screen waypoint markers, each led by its representative: the
    // current item, else the lowest sequence number
    function _markerGroups() {
        const markers = root._waypointMarkers().filter(marker => marker.hasMarker && marker.projected)
        markers.sort((first, second) => {
            const firstCurrent = first.item.isCurrentItem || first.item.hasCurrentChildItem
            const secondCurrent = second.item.isCurrentItem || second.item.hasCurrentChildItem
            if (firstCurrent !== secondCurrent) {
                return firstCurrent ? -1 : 1
            }
            return first.item.sequenceNumber - second.item.sequenceNumber
        })
        if (markers.length === 0) {
            return []
        }
        const points = markers.map(marker => marker.mapToItem(root, marker.anchorPoint.x, marker.anchorPoint.y))
        // A small marker's radius, as MissionItemIndicatorGroup
        const representatives = MapMarkerGrouping.representatives(points, markers[0].smallIndicatorSize / 2)
        const groups = []
        const groupsByRepresentative = {}
        for (let i = 0; i < markers.length; ++i) {
            const representative = representatives[i]
            if (!groupsByRepresentative[representative]) {
                groupsByRepresentative[representative] = []
                groups.push(groupsByRepresentative[representative])
            }
            groupsByRepresentative[representative].push(markers[i])
        }
        return groups
    }

    function _regroup() {
        const markers = root._waypointMarkers()
        for (const marker of markers) {
            marker.collapsed = false
            marker.grouped = false
        }
        if (!root._collapse) {
            return
        }
        for (const group of root._markerGroups()) {
            if (group.length > 1) {
                group[0].grouped = true
                for (let i = 1; i < group.length; ++i) {
                    group[i].collapsed = true
                }
            }
        }
    }

    function _scheduleRegroup() {
        Qt.callLater(root._regroup)
    }

    function _markerClicked(marker) {
        const group = root._markerGroups().find(members => members.includes(marker))
        if (group && group.length > 1) {
            const items = group.map(member => member.item)
            items.sort((first, second) => first.sequenceNumber - second.sequenceNumber)
            root.groupClicked(items, marker)
        } else {
            root.itemClicked(marker.item)
        }
    }

    on_CollapseChanged: {
        _scheduleRegroup()
        groupsInvalidated()
    }
    on_GroupingStateChanged: _scheduleRegroup()

    Connections {
        target: root._camera

        function onDistanceChanged() { root.groupsInvalidated() }
        function onViewportSizeChanged() { root.groupsInvalidated() }
    }

    Connections {
        target: root._collapse ? root._camera : null

        function onDistanceChanged() { root._scheduleRegroup() }
        function onCenterElevationChanged() { root._scheduleRegroup() }
        function onViewportSizeChanged() { root._scheduleRegroup() }
    }

    Repeater {
        id: repeater

        model: root.missionController ? root.missionController.visualItems : 0

        Loader {
            asynchronous: true
            active: object.geoMapVisualQML !== ""

            Component.onCompleted: {
                if (active) {
                    setSource(object.geoMapVisualQML, {
                        item: object,
                        scene: root.scene,
                        surfaceModel: root.surfaceModel
                    })
                }
            }

            onLoaded: {
                // Declared by GeoMapWaypointItem and the landing pattern
                // visuals; wire it up without hardcoding filenames here
                if (item.hasOwnProperty("homeTerrainBias")) {
                    item.homeTerrainBias = Qt.binding(() => root.homeTerrainBias)
                }
                const visual = item
                if (visual.hasOwnProperty("collapsed")) {
                    visual.clicked.connect(() => root._markerClicked(visual))
                    root._scheduleRegroup()
                } else if (visual.clicked !== undefined) {
                    visual.clicked.connect(() => root.itemClicked(object))
                }
            }
        }
    }
}
