import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Corridor Scan Complex Mission Item visuals
TransectStyleMapVisuals {
    id:                 corridorVisuals
    polygonInteractive: false
    hideMapPolygon:     mapPolylineVisuals.dragging
    // The area polygon joins holes to its outer boundary with zero-width bridges, so its outline is drawn
    // from the corridor boundaries instead to keep the bridges hidden.
    polygonBorderWidth: 0

    property bool _currentItem: object.isCurrentItem

    function _updateBoundaryLines() {
        boundaryLineObjects.destroyObjects()
        const boundaries = object.corridorBoundaries
        for (let i = 0; i < boundaries.length; i++) {
            boundaryLineObjects.addObject(boundaryLineComponent.createObject(map, { path: boundaries[i] }), map)
        }
    }

    Component.onCompleted: _updateBoundaryLines()

    Connections {
        target: object
        function onCorridorBoundariesChanged() { corridorVisuals._updateBoundaryLines() }
    }

    QGCDynamicObjectManager {
        id: boundaryLineObjects
    }

    Component {
        id: boundaryLineComponent

        MapPolyline {
            line.color: "black"
            line.width: 1
            visible:    !corridorVisuals.hideMapPolygon
            opacity:    corridorVisuals.opacity
        }
    }

    QGCMapPolylineVisuals {
        id:             mapPolylineVisuals
        mapControl:     map
        mapPolyline:    object.corridorPolyline
        interactive:    _currentItem && parent.interactive
        lineWidth:      3
        lineColor:      QGroundControl.globalPalette.mapMissionTrajectory
        visible:        _currentItem
        opacity:        parent.opacity
    }
}
