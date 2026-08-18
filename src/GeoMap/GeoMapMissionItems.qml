/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

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
Item {
    id: root

    property var missionController
    property var scene
    property var surfaceModel
    property real homeTerrainBias: 0   ///< See GeoMapWaypointItem.homeTerrainBias

    Repeater {
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
            }
        }
    }
}
