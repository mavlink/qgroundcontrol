pragma Singleton

import QtQuick

/// Single source of truth for the Plan View map editing layers. Used by the
/// plan panel tabs and the map visual bindings.
QtObject {
    readonly property int layerPlan:    0
    readonly property int layerMission: 1
    readonly property int layerFence:   2
    readonly property int layerRally:   3
}
