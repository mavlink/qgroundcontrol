import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.PlanView

/// \brief Map visuals for PerimeterScanComplexItem.
///
/// Shows an interactive, editable polygon using the standard QGCMapPolygonVisuals
/// component, which automatically provides the full Polygon Tools toolbar
/// (Basic, Circular, Trace, Load KML, …) when the item is selected.
///
/// A flight-path polyline traces the perimeter in waypoint order so the user
/// can see the direction of travel.

Item {
    id: _root

    property var  map
    property var  vehicle
    property bool interactive: true

    signal clicked(int sequenceNumber)

    property var  _missionItem:  object
    property var  _polygon:      object.perimeterPolygon
    property bool _currentItem:  object.isCurrentItem

    // ----- polygon editing visuals (provides the Polygon Tools toolbar) -----
    QGCMapPolygonVisuals {
        id:               polygonVisuals
        mapControl:       map
        mapPolygon:       _polygon
        interactive:      _currentItem && _root.interactive
        borderWidth:      2
        borderColor:      "#00cc44"
        interiorColor:    "#00cc44"
        interiorOpacity:  0.08 * _root.opacity
    }

    // ----- flight-path polyline (perimeter traversal order) ----------------
    // Shown only when the item is selected so the map stays uncluttered.
    Component {
        id: flightPathComponent

        MapPolyline {
            line.color: "#00cc44"
            line.width: 2
            visible:    _currentItem
            opacity:    _root.opacity

            // Build the closed path from the polygon vertices.
            path: {
                const pts = _polygon.path
                if (pts.length < 2) return []
                // Append first vertex at the end to close the loop visually.
                return pts.concat([pts[0]])
            }
        }
    }

    // ----- entry-point marker ----------------------------------------------
    Component {
        id: entryPointComponent

        MapQuickItem {
            anchorPoint.x: sourceItem.width  / 2
            anchorPoint.y: sourceItem.height / 2
            coordinate:    _missionItem.coordinate
            visible:       _currentItem && _polygon.isValid
            opacity:       _root.opacity
            z:             QGroundControl.zOrderMapItems + 1

            sourceItem: MissionItemIndexLabel {
                checked:     true
                index:       _missionItem.sequenceNumber
                label:       ""
                onClicked:   _root.clicked(_missionItem.sequenceNumber)
            }
        }
    }

    QGCDynamicObjectManager { id: objMgr }

    Component.onCompleted: {
        objMgr.createObjects(
            [flightPathComponent, entryPointComponent],
            map,
            true /* parentObjectIsMap */)
    }

    Component.onDestruction: {
        objMgr.destroyObjects()
    }
}
